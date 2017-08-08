#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <cmd.h>
#include <set.h>
#include <util.h>

static int cmd_insert();
static int cmd_quit();

#define cmd(n, f) { .node = {{.key = n}}, .fun = f, }
struct command commands[] = {
	cmd("q", cmd_quit),
	cmd("i", cmd_insert),
	cmd("p", 0),
};

int
cmd_quit()
{
	return -1;
}

int
cmd_insert()
{
	static char buffer[4096];
	ssize_t length;

	while (true) {
		length = read(0, buffer, 4096);
		if (length == -1) return errno;
		if (!memcmp(buffer, ".\n", 2)) {
			return 0;
		}
		if (!length) return 0;
	}
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
