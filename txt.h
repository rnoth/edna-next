#ifndef _edna_txt_
#define _edna_txt_

#include <stdint.h>

#include <edit.h>
#include <tag.h>

struct piece;

static inline void text_start(struct piece **ctx, struct piece *beg);

struct piece *text_ctor(void);
void text_dtor(struct piece *text);
void text_free(struct piece *text);

int text_delete(struct piece **dest, size_t offset, size_t extent);
int text_insert(struct piece **dest, size_t where,
                struct map *edit, size_t offset, size_t length);

void text_link(struct piece *lef, struct piece *rit);
void text_merge(struct piece **ctx);
struct piece *text_next(struct piece *cur, struct piece *prev);
void text_step(struct piece **links);
void text_unlink(struct piece *lef, struct piece *rit);
size_t text_walk(struct piece **dest, size_t extent);

struct piece {
	uintptr_t link;
	struct map *edit;
	size_t off;
	size_t len;
};

void
text_start(struct piece **t, struct piece *a)
{
	t[0] = a, t[1] = 0;
}

#endif
