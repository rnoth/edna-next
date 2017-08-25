#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include <edna.h>
#include <ext.h>
#include <cmd.h>
#include <mem.h>
#include <txt.h>
#include <util.h>
#include <vec.h>

struct action {
	struct action *chld;
	uintptr_t arg;
	struct piece *prev;
};

struct record {
	struct record *prev;
	struct action *acts;
};

static struct piece *act_free(struct action *act, struct piece *dead);
static struct piece *arrange_pieces(struct piece *chain);
static int add_lines(struct ext *lines, size_t offset, char *buffer, size_t length);
static void free_pieces(struct piece *dead);
static struct piece *kill_piece(struct piece *dead, struct piece *new);
static void edit_dtor(struct map *edit);
static int edit_ctor(struct map *edit);
static void rm_lines(struct ext *lines, size_t offset, size_t extent);
static void revert_insert(struct action *act);

struct piece *
act_free(struct action *cur, struct piece *dead)
{
	struct action *next;
	struct piece *result=0;

	while (cur) {
		next = cur->chld;

		result = untag(cur->arg);
		dead = kill_piece(dead, result);

		free(cur);

		cur = next;
	}

	return dead;
}

struct piece *
arrange_pieces(struct piece *chain)
{
	struct piece *ctx[2];
	struct piece *dead=0;

	ctx[0] = chain, ctx[1] = 0;
	do {
		text_step(ctx);
		dead = kill_piece(dead, ctx[1]);
	} while (text_next(ctx[0], ctx[1]));

	dead = kill_piece(dead, ctx[0]);

	return dead;
}

int
add_lines(struct ext *lines, size_t start, char *buffer, size_t length)
{
	struct ext_node *list;
	struct ext_node *node;
	size_t extent;
	size_t offset;
	char *nl;

	list = 0x0;
	for (offset=0; offset<length; offset+=extent) {
		nl = memchr(buffer+offset, '\n', length-offset);
		extent = nl - buffer - offset + 1;

		node = calloc(1, sizeof *node);
		if (!node) goto fail;

		node->ext = extent;
		node->chld[0] = (uintptr_t)list;
		list = node;
	}

	while (node) {
		list = untag(node->chld[0]);
		ext_insert(lines, start, node);
		node = list;
	}

	return 0;

 fail:
	while (node) {
		list = untag(node->chld[0]);
		free(node);
		node = list;
	}

	return ENOMEM;
}

void
free_pieces(struct piece *dead)
{
	uintptr_t temp;

	while (dead) {
		temp = dead->link;
		free(dead->buffer);
		free(dead);
		dead = untag(temp);
	}
}

void
edit_dtor(struct map *edit)
{
	munmap(edit->map, edit->length);
	close(edit->fd);
}

int
edit_ctor(struct map *edit)
{
	edit->fd = memfd_create("edna-edit");
	if (edit->fd == -1) return errno;

	edit->length = sysconf(_SC_PAGESIZE);
	ftruncate(edit->fd, edit->length);
	edit->map = mmap(0, edit->length, PROT_READ | PROT_WRITE,
	                   MAP_PRIVATE, edit->fd, 0);
	if (edit->map == MAP_FAILED) {
		close(edit->fd);
		return errno;
	}

	return 0;
}

struct piece *
kill_piece(struct piece *dead, struct piece *new)
{
	struct piece *head=dead;

	if (!new) return dead;

	if (!dead) {
		new->link = 0;
		return new;
	}

	if (tag0(dead) > tag0(new)) {
		new->link = tag0(dead);
		return new;
	}

	while (dead) {
		if (dead == new) return head;
		if (dead->link > tag0(new)) {
			new->link = dead->link;
			dead->link = tag0(new);
			return head;
		}

		if (!dead->link) break;
		dead = untag(dead->link);
	}

	new->link = 0;
	dead->link = tag0(new);

	return head;
}

struct piece *
rec_free(struct record *cur, struct piece *dead)
{
	struct record *next;
	struct action *act;

	while (cur) {
		act = cur->acts;
		dead = act_free(act, dead);
		next = cur->prev;
		free(cur);
		cur = next;
	}

	return dead;
}

void
revert_insert(struct action *act)
{
	struct piece *ctx[2];
	struct piece *next;
	struct piece *result;

	ctx[0] = untag(act->arg);
	ctx[1] = act->prev;

	next = text_next(ctx[0], ctx[1]);

	text_unlink(ctx[0], ctx[1]);
	text_unlink(next, ctx[0]);
	text_link(next, ctx[1]);

	result = ctx[0];
	ctx[0] = next;

	result->link = 0;

	text_merge(ctx);

	free(result);
}

void
rm_lines(struct ext *lines, size_t offset, size_t extent)
{
	struct ext_node *dead;
	struct ext_node *list;
	struct ext_node *node;
	size_t start;
	size_t diff;

	start = ext_tell(lines, offset);
	if (start < offset){
		ext_adjust(lines, offset, start - offset);

		diff = offset - start;
		offset += diff;
		extent -= diff;
	}

	list = dead = 0;
	while (extent) {
		node = ext_stab(lines, offset);
		if (node->ext > extent) break;

		dead = ext_remove(lines, offset);

		offset += diff;
		extent -= diff;

		dead->chld[0] = (uintptr_t)list;
		list = dead;
	}

	ext_adjust(lines, offset, -extent);

	while (dead) {
		list = untag(dead->chld[0]);
		free(dead);
		dead = list;
	}

	return;
}

void
edna_fini(struct edna *edna)
{
	struct piece *dead;

	dead = arrange_pieces(edna->chain);
	dead = rec_free(edna->hist, dead);
	free_pieces(dead);
	ext_free(edna->lines);
	edit_dtor(edna->edit);
}

int
edna_init(struct edna *edna)
{
	int err;

	*edna = (struct edna){0};

	edna->hist = calloc(1, sizeof *edna->hist);
	if (!edna->hist) return ENOMEM;

	edna->chain = text_ctor();
	if (!edna->chain) {
		free(edna->hist);
		return ENOMEM;
	}

	err = edit_ctor(edna->edit);
	if (err) {
		free(edna->hist);
		text_dtor(edna->chain);
		return err;
	}

	return 0;
}

int
edna_text_delete(struct edna *edna, size_t offset, size_t extent)
{
	struct action *act;
	struct piece *ctx[2];
	int err;

	act = calloc(1, sizeof *act);
	if (!act) return ENOMEM;

	ctx[0] = edna->chain, ctx[1] = 0;
	err = text_delete(ctx, offset, extent);
	if (err) {
		free(act);
		return err;
	}

	act->arg = tag0(ctx[0]);
	act->chld = edna->hist->acts;

	rm_lines(edna->lines, offset, extent);

	edna->hist->acts = act;

	return 0;
}

int
edna_text_insert(struct edna *edna, size_t offset,
                 char *text, size_t length)
{
	struct action *act;
	struct piece *ctx[2];
	int err;

	act = calloc(1, sizeof *act);
	if (!act) return ENOMEM;

	ctx[0] = edna->chain, ctx[1] = 0;
	err = text_insert(ctx, offset, text, length);
	if (err) {
		free(act);
		return err;
	}

	act->arg = tag1(ctx[0]);
	act->prev = ctx[1];
	act->chld = edna->hist->acts;

	err = add_lines(edna->lines, offset, text, length);
	if (err) {
		revert_insert(act);
		free(act);
		return err;
	}

	edna->hist->acts = act;

	return 0;
}
