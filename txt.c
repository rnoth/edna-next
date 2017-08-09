#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <txt.h>

static void text_link(struct piece *lef, struct piece *rit);
static struct piece *text_next(struct piece *cur, struct piece *next);
static void text_relink(struct piece *, struct piece *, struct piece *);
static int text_split(struct piece **dest, size_t offset);
static void text_unlink(struct piece *lef, struct piece *rit);

struct piece *
text_ctor(void)
{
	struct piece *beg;
	struct piece *end;

	beg = calloc(1, sizeof *beg);
	if (!beg) return 0;
	end = calloc(1, sizeof *end);
	if (!end) {
		free(beg);
		return 0;
	}

	text_link(beg, end);

	return beg;
}

int
text_delete(struct piece **dest, size_t offset, size_t extent)
{
	if (dest[0]->length >= offset + extent) {
		return -1;
	}

	return 0;
}

void
text_dtor(struct piece *txt)
{
	struct piece *next=txt;
	struct piece *cur=0;
	struct piece *prev;


	do {
		prev = cur, cur = next;
		next = text_next(cur, prev);
		free(cur);
	} while (next);
}

int
text_insert(struct piece **dest, struct piece *new, size_t offset)
{
	int err;

	err = text_split(dest, offset);
	if (err) return err;

	text_relink(dest[0], new, dest[1]);

	return 0;
}

void
text_link(struct piece *lef, struct piece *rit)
{
	if (lef) lef->link ^= (link)rit;
	if (rit) rit->link ^= (link)lef;
}

struct piece *
text_next(struct piece *cur, struct piece *prev)
{
	uintptr_t res;

	res = cur->link ^ (link)prev;

	return (void *)res;
}


void
text_relink(struct piece *next, struct piece *new, struct piece *prev)
{
	text_unlink(next, prev);
	text_link(next, new);
	text_link(new, prev);
}

int
text_split(struct piece **dest, size_t offset)
{
	struct piece *next;
	struct piece *new;

	if (!offset || offset >= dest[0]->length) {
		next = text_next(dest[0], dest[1]);
		dest[1] = dest[0];
		dest[0] = next;

		return 0;
	}

	new = calloc(1, sizeof *new);
	if (!new) return ENOMEM;

	new->length = dest[0]->length - offset;
	new->buffer = dest[0]->buffer + offset;

	dest[0]->length = offset;

	next = text_next(dest[0], dest[1]);

	text_relink(next, new, dest[0]);

	dest[1] = dest[0];
	dest[0] = new;

	return 0;
}

void
text_unlink(struct piece *lef, struct piece *rit)
{
	lef->link ^= (link)rit;
	rit->link ^= (link)lef;
}

size_t
text_walk(struct piece **dest, size_t extent)
{
	struct piece *next;
	size_t offset = 0;

	while (offset < extent) {
		next = text_next(dest[0], dest[1]);
		if (!next) break;
		if (offset + dest[0]->length >= extent) {
			break;
		}
		offset += dest[0]->length;
		dest[1] = dest[0];
		dest[0] = next;
	}

	return extent - offset;
}
