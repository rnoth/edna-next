#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <cmd.h>
#include <fd.h>
#include <set.h>
#include <txt.h>
#include <util.h>

static int cmd_insert();
static int cmd_print();
static int cmd_quit();

static size_t cursor[2] = {0, 0};

#define cmd(n, f, a) { .node = {{.key = n}}, .fun = f, .arg = a }

struct command commands[] = {
	cmd("q", cmd_quit, 0),
	cmd("i", cmd_insert, cursor),
	cmd("p", cmd_print, cursor),
};

int
cmd_insert(struct edna *edna, size_t *cursor)
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
cmd_quit()
{
	return -1;
}

int
cmd_print(struct edna *edna, size_t *cursor)
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
cmd_add(struct set *cmds, struct command *cmd)
{
	set_add(cmds, cmd->node, cmd->len);
}

void
cmd_init(struct set *cmds)
{
	size_t i;

	for (i=0; i<arr_len(commands); ++i) {
		commands[i].len = strlen(commands[i].node->key) + 1;
		cmd_add(cmds, commands + i);
	}
}

struct command *
cmd_lookup(struct set *cmds, char *name, size_t len)
{
	struct command *res;

	res = set_query(cmds, name, len);
	return !strncmp(res->node->key, name, len) ? res : 0;
}
