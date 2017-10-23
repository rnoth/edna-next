#ifndef _edna_frag_
#define _edna_frag_
#include <stddef.h>
#include <stdint.h>

struct frag;

void frag_append(struct frag *hint_node, size_t off, struct frag *new_node);
void frag_delete(struct frag *del_node);
void *frag_get_root(struct frag *node);
void frag_insert(struct frag *hint_node, size_t off, struct frag *new_node);
void *frag_next(struct frag *node, int direction);
void frag_offset(struct frag *targ_node, size_t by);
void frag_remove(struct frag *del_node);
void *frag_stab(struct frag *hint_node, size_t *point);
void frag_trunc(struct frag *node, size_t off);
void *frag_query(struct frag *hint_node, size_t point);

struct frag {
	size_t    len;

	uintptr_t link[3];
	size_t    off;
	size_t    max;
	size_t    pop;
};

#endif
