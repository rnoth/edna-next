#ifndef _edna_cmd_
#define _edna_cmd_

#include <set.h>

struct command;

void cmd_add(struct set *cmds, struct command *cmd);
void cmd_init(struct set *cmds);
struct command *cmd_lookup(struct set *cmds, char *name, size_t len);

struct command {
	struct set_node *node;
	char *name;
	size_t len;
	int (*fun)(void *ctx);
	void *ctx;
};

#endif
