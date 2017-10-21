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
	int err;

	len = read(0, buffer, 4096);
	if (len == -1) return errno;
	if (!len) return -1;

	if (len > 1 && !memcmp(buffer, ".\n", 2)) {
		return -1;
	}

	p = edna->lines;

	err = edna_text_insert(edna, edna->dot[0] + edna->dot[1], buffer, len);
	if (err) return err;

	edna->dot[0] += p ? p->len : 0;
	edna->dot[1] = edna->lines->len;

	return 0;
}

int
edna_cmd_back(struct edna *edna)
{
	struct frag *p;

	if (!edna->dot[0]) {
		edna_fail(edna, "beginning of file");
		return 0;
	}

	p = frag_prev(edna->lines);
	if (!p) __builtin_trap();
	edna->lines = frag_prev(edna->lines);
	edna->dot[0] -= edna->dot[1] = edna->lines->len;

	return 0;
}

int
edna_cmd_forth(struct edna *edna)
{
	struct ext_node *p;

	p = frag_next(edna->lines);
	if (!p) {
		edna_fail(edna, "end of file");
		return 0;
	}

	edna->dot[0] += edna->dot[1];
	edna->dot[1] = p->ext;

	return 0;
}

int
edna_cmd_delete(struct edna *edna)
{
	if (!edna->dot[1]) {
		edna_fail(edna, "empty selection");
		return 0;
	}

	return edna_text_delete(edna, edna->dot[0], edna->dot[1]);
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

	if (off) {
		txt = ctx[0]->edit->map + ctx[0]->offset + off;
		min = umin(ctx[0]->length - off, ext);
		write(1, txt, min);

		ext -= min;
		text_step(ctx);
	}
	while (ext) {
		txt = edna->edit->map + ctx[0]->offset + off;
		min = umin(ctx[0]->length, ext);
		write(1, txt, min);

		ext -= min;
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
