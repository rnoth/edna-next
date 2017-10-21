#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <cmd.h>
#include <err.h>
#include <fd.h>
#include <frag.h>
#include <set.h>
#include <txt.h>
#include <util.h>

int
do_insert(struct edna *edna)
{
	static char buffer[4096];
	struct frag *p;
	ssize_t len;

	len = read(0, buffer, 4096);
	if (len == -1) return errno;
	if (!len) return -1;

	if (len > 1 && !memcmp(buffer, ".\n", 2)) {
		return -1;
	}

	p = edna->lines;

	return edna_text_insert(edna, edna->dot[0] + edna->dot[1], buffer, len);
}

int
edna_cmd_back(struct edna *a)
{
	struct frag *p;

	p = frag_next(a->lines, 0);
	if (!p) {
		edna_fail(a, "beginning of file");
		return 0;
	}
	a->lines = p;
	a->dot[0] -= p->len;
	a->dot[1] = p->len;

	return 0;
}

int
edna_cmd_forth(struct edna *a)
{
	struct frag *p;

	p = frag_next(a->lines, 1);
	if (!p) {
		edna_fail(a, "end of file");
		return 0;
	}

	a->lines = p;
	a->dot[0] += a->dot[1];
	a->dot[1] = p->len;

	return 0;
}

int
edna_cmd_delete(struct edna *a)
{
	if (!a->dot[1]) {
		edna_fail(a, "empty selection");
		return 0;
	}

	return edna_text_delete(a, a->dot[0], a->dot[1]);
}

int
edna_cmd_insert(struct edna *a)
{
	int err=0;
	while (!err) err = do_insert(a);

	if (err > 0) return err;
	else return 0;
}

int
edna_cmd_print(struct edna *edna)
{
	struct piece *ctx[2];
	size_t ext = edna->dot[1];
	size_t off = edna->dot[0];
	size_t min;
	char *txt;

	if (!edna->dot[1]) {
		edna_fail(edna, "empty selection");
		return 0;
	}

	text_start(ctx, edna->chain);
	off = text_walk(ctx, off);

	while (ext) {
		txt = ctx[0]->edit->map + ctx[0]->offset + off;
		min = umin(ctx[0]->length - off, ext);
		write(1, txt, min);

		ext -= min, off=0;
		text_step(ctx);
	}

	return 0;
}

void
edna_add_cmd(struct edna *edna, struct command *cmd)
{
	set_add(edna->cmds, cmd->node, cmd->len);
}

struct command *
edna_lookup_cmd(struct edna *edna, char *name, size_t len)
{
	struct command *res;

	res = set_query(edna->cmds, name, len);
	return !strncmp(res->node->key, name, len) ? res : 0;
}
