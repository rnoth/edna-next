#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include <edna.h>
#include <ext.h>
#include <cmd.h>
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

static int add_lines(struct ext *lines, size_t offset, char *buffer, size_t length);
static void rm_lines(struct ext *lines, size_t offset, size_t extent);
static void revert_insert(struct action *act);

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
		extent = nl - buffer + 1;

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
rm_lines(struct ext *lines, size_t offset, size_t extent)
{
	struct ext_node *dead;
	struct ext_node *list=0x0;
	size_t start;
	size_t diff;

	start = ext_tell(lines, offset);
	ext_adjust(lines, offset, start - offset);

	diff = offset - start;
	offset += diff;
	extent -= diff;

	while (extent) {
		diff = ext_tell(lines, offset);
		if (diff < extent) break;

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
rec_free(struct record *rec)
{
	struct action *cur_act;
	struct action *next_act;
	struct record *next_rec;

	while (rec) {
		cur_act = rec->acts;
		while (cur_act) {
			next_act = cur_act->chld;
			free(cur_act);
			cur_act = next_act;
		}
		next_rec = rec->prev;
		free(rec);
		rec = next_rec;
	}
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
edna_fini(struct edna *edna)
{
	text_free(edna->chain);
	text_free(edna->dead);
	rec_free(edna->hist);
	ext_free(edna->lines);
}

int
edna_init(struct edna *edna)
{
	*edna = (struct edna){0};

	edna->hist = calloc(1, sizeof *edna->hist);
	if (!edna->hist) return ENOMEM;

	edna->chain = text_ctor();
	if (!edna->chain) {
		free(edna->hist);
		return ENOMEM;
	}

	*edna->cmds = (struct set){0};
	cmd_init(edna->cmds);

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
edna_text_insert(struct edna *edna, size_t offset, char *text, size_t length)
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
