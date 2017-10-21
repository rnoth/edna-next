#ifndef _edna_frag_
#define _edna_frag_
#include <stddef.h>
#include <stdint.h>

struct frag;

void frag_append(struct frag *hint_node, size_t off, struct frag *new_node);
void frag_delete(struct frag *del_node);
void *frag_get_root(struct frag *cur_node);
void frag_insert(struct frag *hint_node, size_t off, struct frag *new_node);
void *frag_next(struct frag *cur_node, int direction);
void frag_offset(struct frag *targ_node, size_t by);
void *frag_stab(struct frag *hint_node, size_t *point);
void *frag_query(struct frag *hint_node, size_t point);

struct frag {
	uintptr_t link[3];
	size_t    off;
	size_t    max;
	size_t    len;
};

#endif
