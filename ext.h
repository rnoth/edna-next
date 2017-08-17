#ifndef _edna_ext_
#define _edna_ext_

#include <stddef.h>
#include <stdint.h>

struct ext_node {
	uintptr_t chld[2];
	size_t off;
	size_t ext;
};

struct ext {
	uintptr_t root;
	size_t off;
	size_t len;
};

void ext_append(struct ext *ext, struct ext_node *new_node);
void ext_insert(struct ext *ext, struct ext_node *new_node, size_t offset);
void *ext_stab(struct ext *ext, size_t point);


#endif
