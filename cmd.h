#ifndef _edna_cmd_
#define _edna_cmd_

#include <edna.h>
#include <set.h>

struct command;

void edna_add_cmd(struct edna *edna, struct command *cmd);
struct command *edna_lookup_cmd(struct edna *edna, char *name, size_t len);

int edna_cmd_back();
int edna_cmd_forth();
int edna_cmd_insert();
int edna_cmd_print();

struct command {
	struct set_node node[1];
	size_t len;
	int (*fun)(struct edna *edna, void *arg);
	void *arg;
};

#endif
