#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <cmd.h>
#include <err.h>
#include <fd.h>
#include <set.h>
#include <txt.h>
#include <util.h>

int
edna_cmd_back(struct edna *edna, size_t *cursor)
{
	struct ext_node *node;

	if (!cursor[0]) {
		edna_fail(edna, "beginning of file");
		return 0;
	}

	node = ext_stab(edna->lines, cursor[0] - 1);
	cursor[0] -= cursor[1] = node->ext;

	return 0;
}

int
edna_cmd_forth(struct edna *edna, size_t *cursor)
{
	struct ext_node *node;
	size_t end;

	end = cursor[0] + cursor[1];

	node = ext_stab(edna->lines, end);
	if (!node) {
		edna_fail(edna, "end of file");
		return 0;
	}

	cursor[0] = end;
	cursor[1] = node->ext;

	return 0;
}

int
edna_cmd_delete(struct edna *edna, size_t *cursor)
{
	struct ext_node *node;
	int err;

	if (!cursor[1]) {
		edna_fail(edna, "empty selection");
		return 0;
	}

	err = edna_text_delete(edna, cursor[0], cursor[1]);
	if (err) return err;

	node = ext_stab(edna->lines, cursor[0]);

	if (node) {
		cursor[1] = node->ext;
		return 0;
	}

	if (!cursor[0]) {
		cursor[1] = 0;
		return 0;
	}

	node = ext_stab(edna->lines, cursor[0] - 1);

	cursor[0] -= node->ext;
	cursor[1] = node->ext;

	return 0;
}

int
edna_cmd_insert(struct edna *edna, size_t *cursor)
{
	struct ext_node *node;
	static char buffer[4096];
	ssize_t len;
	size_t end;
	int err;

 again:
	len = read(0, buffer, 4096);
	if (len == -1) return errno;
	if (!len) return 0;

	if (len > 1 && !memcmp(buffer, ".\n", 2)) {
		return 0;
	}

	err = edna_text_insert(edna, cursor[0] + cursor[1], buffer, len);
	if (err) return err;

	end = cursor[0] + cursor[1] + len;
	node = ext_stab(edna->lines, end - 1);
	cursor[0] = ext_tell(edna->lines, end - 1);
	cursor[1] = node->ext;

	goto again;
}

int
edna_cmd_print(struct edna *edna, size_t *cursor)
{
	struct piece *ctx[2];
	size_t ext = cursor[1];
	size_t off = cursor[0];
	size_t min;
	char *text;

	if (!cursor[1]) {
		edna_fail(edna, "empty selection");
		return 0;
	}

	text_start(ctx, edna->chain);
	off = text_walk(ctx, off);
	text = edna->edit->map + ctx[0]->offset + off;
	min = umin(ctx[0]->length - off, ext);
	write(1, text, min);

	ext -= min;

	while (ext) {
		text_step(ctx);
		text = edna->edit->map + ctx[0]->offset;
		min = umin(ctx[0]->length, ext);
		write(1, text, min);

		ext -= min;

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
