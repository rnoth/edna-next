#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include <edna.h>

typedef uintptr_t link;

struct piece {
	uintptr_t link;
	size_t length;
	char *buffer;
};

static struct piece *text_ctor(void);
static void text_dtor(struct piece *text);
static int text_insert(struct piece **dest, struct piece *new, size_t offset);
static void text_link(struct piece *lef, struct piece *rit);
static struct piece *text_next(struct piece *cur, struct piece *next);
static void text_relink(struct piece *, struct piece *, struct piece *);
static int text_split(struct piece **dest, size_t offset);
static void text_unlink(struct piece *lef, struct piece *rit);
static size_t text_walk(struct piece **dest, size_t extent);

void
edna_fini(struct edna *edna)
{
	text_dtor(edna->text);
}

int
edna_init(struct edna *edna)
{
	edna->text = text_ctor();
	if (!edna->text) return ENOMEM;

	return 0;
}

int
edna_text_insert(struct edna *edna, size_t offset, char *text, size_t length)
{
	struct piece *links[2];
	struct piece *pie;

	pie = calloc(1, sizeof *pie);
	if (!pie) return ENOMEM;

	pie->buffer = text;
	pie->length = length;

	links[0] = edna->text, links[1] = 0;
	offset = text_walk(links, offset);
	text_insert(links, pie, offset);

	return 0;
}

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

void
text_dtor(struct piece *txt)
{
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

