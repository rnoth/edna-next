#ifndef _edna_cmd_
#define _edna_cmd_

#include <set.h>

struct command;

int cmd_add(struct set *set, struct command *cmd);

struct command {
	struct set_node *node;
	char *name;
	int (*fun)(void *ctx);
	void *ctx;
};

#endif
