#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <cmd.h>
#include <set.h>
#include <txt.h>
#include <util.h>

static int cmd_insert();
static int cmd_print();
static int cmd_quit();

static size_t cursor[2] = {0, 1};

#define cmd(n, f, a) { .node = {{.key = n}}, .fun = f, .arg = a }

struct command commands[] = {
	cmd("q", cmd_quit, 0),
	cmd("i", cmd_insert, cursor),
	cmd("p", cmd_print, cursor),
};

int
cmd_insert(struct edna *edna, size_t *offset)
{
	static char buffer[4096];
	char *ln;
	ssize_t length;
	int err;

	while (true) {
		length = read(0, buffer, 4096);
		if (length == -1) return errno;

		if (!memcmp(buffer, ".\n", 2)) {
			return 0;
		}
		if (!length) return 0;

		ln = malloc(length);
		memcpy(ln, buffer, length);

		err = edna_text_insert(edna, offset[1], ln, length);
		if (err) return err;

		offset[0] = offset[1] - 1;
		offset[1] += length;
	}
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
	size_t offset = cursor[0];

	links[0] = edna->text, links[1] = 0;
	text_walk(links, cursor[0]);

	while (offset < cursor[1]) {
		text_step(links);

		write(1, links[0]->buffer, links[0]->length);
		offset += links[0]->length;
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
