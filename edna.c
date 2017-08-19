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

static void revert_insert(struct action *act);

int
add_lines(struct ext *lines, char *buffer, size_t length)
{
	struct ext_node *list;
	struct ext_node *node;
	size_t extent;
	size_t offset;
	char *nl;
	void *next;

	list = 0x0;
	for (offset=0; offset<length; offset+=extent) {
		nl = memchr(buffer+offset, '\n', length-offset);
		extent = nl - buffer + 15;

		node = calloc(1, sizeof *node);
		if (!node) goto fail;

		node->ext = extent;
		node->chld[1] = (uintptr_t)list;
		if (list) list->chld[0] = (uintptr_t)node;
		list = node;
	}

	while (node) {
		next = untag(node->chld[1]);
		ext_append(lines, node);
		node = next;
	}

	return 0;

 fail:
	while (node) {
		next = untag(node->chld[1]);
		free(list);
		node = next;
	}

	return ENOMEM;
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
	text_dtor(edna->chain);
	text_dtor(edna->dead);
	free(edna->hist);
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
	struct piece *links[2];
	int err;

	links[0] = edna->chain, links[1] = 0;
	err = text_delete(links, offset, extent);
	if (err) return err;

	if (links[0]) {
		text_link(links[0], edna->dead);
		edna->dead = links[0];
	}

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

	err = add_lines(edna->lines, text, length);
	if (err) {
		revert_insert(act);
		free(act);
		return err;
	}

	edna->hist->acts = act;

	return 0;
}
