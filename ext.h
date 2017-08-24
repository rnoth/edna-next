#ifndef _edna_ext_
#define _edna_ext_

#include <stddef.h>
#include <stdint.h>

struct ext {
	uintptr_t root;
	size_t off;
	size_t len;
};

struct ext_node {
	uintptr_t chld[2];
	size_t off;
	size_t ext;
};

struct ext_walker {
	uintptr_t prev;
	uintptr_t tag;
	ptrdiff_t adj;
	size_t off;
	size_t len;
};

void ext_adjust(struct ext *ext, size_t offset, ptrdiff_t adjust);
void ext_append(struct ext *ext, struct ext_node *new);
void ext_free(struct ext *ext);
void ext_insert(struct ext *ext, size_t offset, struct ext_node *new);
void ext_offset(struct ext *ext, size_t offset, ptrdiff_t adjust);
void *ext_remove(struct ext *ext, size_t offset);
void *ext_stab(struct ext *ext, size_t offset);
size_t ext_tell(struct ext *ext, size_t offset);
void *ext_walk(struct ext_walker *walker, size_t offset);


#endif
