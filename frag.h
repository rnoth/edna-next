#ifndef _edna_frag_
#define _edna_frag_
#include <stddef.h>
#include <stdint.h>

struct frag;
struct frag_node;

void frag_delete(struct frag *fg, struct frag_node *node);
void frag_flush(struct frag *fg);
int frag_insert(struct frag *fg, size_t off, struct frag_node *node);
void *frag_stab(struct frag *fg, size_t pos);

struct frag {
	uintptr_t cur;
	size_t off;
	size_t rem;
};

struct frag_node {
	uintptr_t link[3];
	size_t off[2];
	size_t len;
};

#endif
