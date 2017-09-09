#ifndef _edna_frag_
#define _edna_frag_
#include <stddef.h>

typedef size_t edna_pos;

struct frag;

void *frag_search(struct frag *fg, size_t pos);

struct frag {
	uintptr_t prnt;
	uintptr_t chld;
};

#endif
