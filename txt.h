#ifndef _edna_txt_
#define _edna_txt_

#include <stdint.h>

#include <ext.h>
#include <tag.h>

struct piece {
	struct ext_node node[1];
	uintptr_t link;
	size_t length;
	char *buffer;
};

struct piece *text_ctor(void);
void text_dtor(struct piece *text);

int text_delete(struct piece **dest, size_t offset, size_t extent);
int text_insert(struct piece **dest, size_t offset, char *buffer, size_t length);

void text_link(struct piece *lef, struct piece *rit);
void text_merge(struct piece **ctx);
struct piece *text_next(struct piece *cur, struct piece *prev);
void text_step(struct piece **links);
void text_unlink(struct piece *lef, struct piece *rit);
size_t text_walk(struct piece **dest, size_t extent);

#endif
