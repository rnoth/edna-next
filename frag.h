
#ifndef _edna_frag_
#define _edna_frag_
#include <stddef.h>
#include <stdint.h>

typedef size_t edna_pos;

struct frag;
struct frag_node;

void frag_delete(struct frag *fg, size_t pos);
void frag_flush(struct frag *fg);
int frag_insert(struct frag *fg, struct frag_node *node);
void *frag_stab(struct frag *fg, size_t pos);

struct frag {
	uintptr_t prnt;
	uintptr_t chld;

	size_t dsp;
};

struct frag_node {
	uintptr_t link[2];
	size_t dsp;
	size_t wid;

	size_t off;
	size_t len;
};

#endif
