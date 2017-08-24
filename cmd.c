#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <cmd.h>
#include <fd.h>
#include <set.h>
#include <txt.h>
#include <util.h>

int
edna_cmd_back(struct edna *edna, size_t *cursor)
{
	struct ext_node *ln;

	if (!cursor[0]) return -1;

	ln = ext_stab(edna->lines, cursor[0] - 1);
	cursor[0] -= cursor[1] = ln->ext;

	return 0;
}

int
edna_cmd_forth(struct edna *edna, size_t *cursor)
{
	struct ext_node *ln;
	size_t end;

	end = cursor[0] + cursor[1];

	ln = ext_stab(edna->lines, end);
	if (!ln) return 0;

	cursor[0] = end;
	cursor[1] = ln->ext;

	return 0;
}

int
edna_cmd_insert(struct edna *edna, size_t *cursor)
{
	struct read ln[1]={0};
	int err;

 again:
	err = fd_read(ln, 0);
	if (err == -1) return 0;
	if (err) return err;

	if (!ln->length) {
		return 0;
	}

	if (ln->length == 2 && !memcmp(ln->buffer, ".\n", 2)) {
		free(ln->buffer);
		return 0;
	}

	err = edna_text_insert(edna, cursor[0] + cursor[1],
	                       ln->buffer, ln->length);
	if (err) {
		free(ln->buffer);
		return 0;
	}

	cursor[0] += cursor[1];
	cursor[1] = ln->length;

	goto again;
}

int
edna_cmd_quit()
{
	return -1;
}

int
edna_cmd_print(struct edna *edna, size_t *cursor)
{
	struct piece *links[2];
	size_t end = cursor[0] + cursor[1];
	size_t off = cursor[0];
	size_t min;

	links[0] = edna->chain, links[1] = 0;
	text_walk(links, cursor[0]);

	while (off < end) {
		min = umin(links[0]->length, end - off);
		write(1, links[0]->buffer, min);
		off += links[0]->length;

		text_step(links);
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
