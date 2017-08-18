#ifndef _edna_txt_
#define _edna_txt_

#include <stdint.h>

#include <ext.h>

struct piece {
	struct ext_node node[1];
	uintptr_t link;
	size_t length;
	char *buffer;
};

struct piece *text_ctor(void);
int text_delete(struct piece **dest, size_t offset, size_t extent);
void text_dtor(struct piece *text);
int text_insert(struct piece **dest, struct piece *new, size_t offset);
void text_link(struct piece *lef, struct piece *rit);
struct piece *text_next(struct piece *cur, struct piece *prev);
void text_step(struct piece **links);
size_t text_walk(struct piece **dest, size_t extent);

#endif
