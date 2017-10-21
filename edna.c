#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cmd.h>
#include <edna.h>
#include <ext.h>
#include <file.h>
#include <ln.h>
#include <mem.h>
#include <txt.h>
#include <util.h>

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
static void free_pieces(struct piece *dead);
static struct piece *kill_piece(struct piece *dead, struct piece *new);
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

void
free_pieces(struct piece *dead)
{
	uintptr_t temp;

	while (dead) {
		temp = dead->link;
		free(dead);
		dead = untag(temp);
	}
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
edna_fini(struct edna *e)
{
	struct piece *p;

	p = arrange_pieces(e->chain);
	p = rec_free(e->hist, p);
	free_pieces(p);
	edit_dtor(e->edit);
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
edna_file_open(struct edna *edna, size_t offset, char *fn)
{
	struct piece *ctx[2];
	int err;

	err = filemap_ctor(edna->file, fn);
	if (err) return err;

	text_start(ctx, edna->chain);
	err = text_insert(ctx, offset, edna->file, 0, edna->file->length);
	if (err) return err;

	return ln_insert(&edna->lines, offset, edna->file->map, edna->file->length);
}

int
edna_text_delete(struct edna *edna, size_t offset, size_t extent)
{
	struct action *act;
	struct piece *ctx[2];
	int err;

	act = calloc(1, sizeof *act);
	if (!act) return ENOMEM;

	text_start(ctx, edna->chain);
	err = text_delete(ctx, offset, extent);
	if (err) {
		free(act);
		return err;
	}

	act->arg = tag0(ctx[0]);
	act->chld = edna->hist->acts;

	ln_delete(&edna->lines, offset, extent);

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

	ctx[0] = edna->chain, ctx[1] = 0;
	err = text_insert(ctx, offset, edna->edit, edna->edit->offset, length);
	if (err) return err;

	act = calloc(1, sizeof *act);
	if (!act) return ENOMEM;
	act->arg = tag1(ctx[0]);
	act->prev = ctx[1];
	act->chld = edna->hist->acts;

	err = edit_append(edna->edit, text, length);
	if (err) {
		revert_insert(act);
		free(act);
		return err;
	}

	err = ln_insert(&edna->lines, offset, text, length);
	if (err) {
		revert_insert(act);
		free(act);
		return err;
	}

	edna->hist->acts = act;

	return 0;
}
