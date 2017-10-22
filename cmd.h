#ifndef _edna_cmd_
#define _edna_cmd_

#include <edna.h>
#include <set.h>

struct command;

void edna_add_cmd(struct edna *edna, struct command *cmd);
struct command *edna_lookup_cmd(struct edna *edna, char *name, size_t len);

int cmd_back();
int cmd_forth();
int cmd_delete();
int cmd_insert();
int cmd_print();

struct command {
	struct set_node node[1];
	size_t len;
	int (*fun)(struct edna *edna, void *arg);
	void *arg;
};

#endif
