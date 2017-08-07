#include <string.h>

#include <cmd.h>
#include <set.h>
#include <util.h>

struct command commands[] = {
	{.name = "q", .fun = 0, },
	{.name = "i", .fun = 0, },
	{.name = "p", .fun = 0, },
};

void
cmd_add(struct set *cmds, struct command *cmd)
{
	set_add_key(cmds, cmd->node, cmd->name, cmd->len);
}

void
cmd_init(struct set *cmds)
{
	size_t i;

	for (i=0; i<arr_len(commands); ++i) {
		commands[i].len = strlen(commands[i].name) + 1;
		cmd_add(cmds, commands + i);
	}
}

struct command *
cmd_lookup(struct set *cmds, char *name, size_t len)
{
	struct command *res;

	res = set_query(cmds, name, len);

	if (res->len != len) return 0;

	return !memcmp(res->name, name, len) ? res : 0;
}
