#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <txt.h>

static void text_delete_across(struct piece **ctx, size_t offset, size_t extent);
static int text_delete_within(struct piece **ctx, size_t offset, size_t extent);
static void text_relink(struct piece *, struct piece *, struct piece *);
static int text_split(struct piece **ctx, size_t offset, size_t extent);

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
text_step(struct piece **links)
{
	struct piece *next;
	next = text_next(links[0], links[1]);
	links[1] = links[0], links[0] = next;
}

int
text_delete(struct piece **ctx, size_t offset, size_t extent)
{
	if (offset >= ctx[0]->length) offset = text_walk(ctx, offset);

	if (offset + extent < ctx[0]->length) {
		return text_delete_within(ctx, offset, extent);
	}

	text_delete_across(ctx, offset, extent);
	return 0;
}

void
text_delete_across(struct piece **ctx, size_t offset, size_t extent)
{
	struct piece *end[2];

	if (offset) {
		extent -= ctx[0]->length - offset;
		ctx[0]->length = offset;
		offset = 0;

		text_step(ctx);
	}

	end[0] = ctx[0], end[1] = ctx[1];

	if (extent >= ctx[0]->length) {
		extent = text_walk(end, extent);

		text_unlink(ctx[0], ctx[1]);
		text_unlink(end[0], end[1]);

		text_link(end[0], ctx[1]);
		end[1] = ctx[1];

	} else ctx[0] = 0;
	ctx[1] = 0;

	if (extent) {
		end[0]->length -= extent;
		end[0]->buffer += extent;
	}

	text_merge(end);
}

int
text_delete_within(struct piece **ctx, size_t offset, size_t extent)
{
	int err;

	err = text_split(ctx, offset, extent);
	if (err) return err;

	ctx[0] = 0, ctx[1] = 0;
	return 0;
}

void
text_dtor(struct piece *txt)
{
	struct piece *next=txt;
	struct piece *cur=0;
	struct piece *prev;

	while (next) {
		prev = cur, cur = next;
		next = text_next(cur, prev);
		free(cur);
	}
}

void
text_free(struct piece *text)
{
	struct piece *next;
	struct piece *prev=0;

	while (text) {
		next = text_next(text, prev);
		free(text->buffer);
		free(text);
		prev=text;
		text=next;
	}
}

int
text_insert(struct piece **dest, size_t offset, char *buffer, size_t length)
{
	struct piece *new;
	int err;

	new = calloc(1, sizeof *new);
	if (!new) return ENOMEM;

	new->buffer = buffer;
	new->length = length;

	offset = text_walk(dest, offset);

	err = text_split(dest, offset, 0);
	if (err) {
		free(new);
		return err;
	}

	text_relink(dest[0], new, dest[1]);
	dest[0] = new;

	return 0;

}

void
text_link(struct piece *lef, struct piece *rit)
{
	if (lef) lef->link ^= (uintptr_t)rit;
	if (rit) rit->link ^= (uintptr_t)lef;
}

void
text_merge(struct piece **ctx)
{
	struct piece *dead;
	struct piece *next;
	char *end;

	if (!ctx[1]->buffer) return;

	end = ctx[1]->buffer + ctx[1]->length;
	if (end != ctx[0]->buffer) return;

	ctx[1]->length += ctx[0]->length;

	next = text_next(ctx[0], ctx[1]);
	text_unlink(ctx[0], ctx[1]);
	text_unlink(next, ctx[0]);
	text_link(next, ctx[1]);

	dead = ctx[0];
	ctx[1] = next;

	free(dead);
}

struct piece *
text_next(struct piece *cur, struct piece *prev)
{
	uintptr_t res;

	res = cur->link ^ (uintptr_t)prev;

	return (void *)res;
}

void
text_relink(struct piece *next, struct piece *new, struct piece *prev)
{
	if (next && prev) text_unlink(next, prev);
	text_link(next, new);
	text_link(new, prev);
}

int
text_split(struct piece **dest, size_t offset, size_t extent)
{
	struct piece *next;
	struct piece *new;

	if (offset + extent == 0) return 0;

	new = calloc(1, sizeof *new);
	if (!new) return ENOMEM;

	new->length = dest[0]->length - offset - extent;
	new->buffer = dest[0]->buffer + offset + extent;

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
	lef->link ^= (uintptr_t)rit;
	rit->link ^= (uintptr_t)lef;
}

size_t
text_walk(struct piece **dest, size_t extent)
{
	struct piece *next;
	size_t offset = 0;

	do {
		next = text_next(dest[0], dest[1]);
		if (!next) break;

		if (offset + dest[0]->length > extent) {
			break;
		}
		offset += dest[0]->length;
		dest[1] = dest[0];
		dest[0] = next;
	} while (offset < extent);

	return extent - offset;
}
