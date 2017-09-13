#include <stdint.h>
#include <stdlib.h>

#include <frag.h>
#include <util.h>
#include <tag.h>

enum link {
	left,
	right,
	up,
};

static struct frag_node *frag_find(struct frag *, size_t pos);
static enum link frag_cmp(struct frag_node *node, ptrdiff_t pos); 

void
add_chld(uintptr_t d, uintptr_t c, enum link k)
{
	struct frag_node *dd;
	struct frag_node *cc;

	dd = untag(d);
	cc = untag(c);
	dd->link[ k] = c;
	cc->link[!k] = d;
}

void
add_link(struct frag_node *node, uintptr_t tag, enum link k)
{
	node->link[k] = tag;
}

uintptr_t
get_link(struct frag_node *node, enum link k)
{
	return node->link[k];
}

enum link
frag_cmp(struct frag_node *node, ptrdiff_t pos)
{
	if (pos < node->dsp) return up;

	if (pos > node->off) return left;
	if (pos < node->wid) return right;
	else return up;
}

void
frag_delete(struct frag *fg, size_t pos)
{
	struct frag_node *match;

	match = frag_stab(fg, pos);
	if (!match) return;

	fg->cur = 0;
}

struct frag_node *
frag_find(struct frag *fg, size_t pos)
{
	struct frag_node *node;
	uintptr_t next;
	enum link k;

	node = untag(fg->cur);

	while (!in_range(node->off, node->len, pos - fg->dsp)) {

		k = frag_cmp(node, pos - fg->dsp);

		next = get_link(node, k);

		if (!next) return node;

		if (k == right) fg->dsp += node->dsp;

		node = untag(next);

		if (k == up && node->link[right] == fg->cur) {
			fg->dsp -= node->dsp;
		}

		fg->cur = next;
	}

	return node;
}

void
frag_flush(struct frag *fg)
{
	__builtin_trap();
}

int
frag_insert(struct frag *fg, struct frag_node *node)
{
	struct frag_node *match;
	enum link k;

	if (!fg->cur) {
		fg->cur = (uintptr_t)node;
		*node = (struct frag_node){
			.link = {0, 0},
			.off = node->off,
			.len = node->len,
			.dsp = node->len,
			.wid = 0,
		};

		return 0;
	}

	match = frag_find(fg, node->off);

	k = node->off - fg->dsp >= match->off;

	add_chld(fg->cur, (uintptr_t)node, k);

	if (k) match->wid += node->len;
	else match->off += node->len, match->dsp += node->len;

	node->off = 0;

	fg->cur = (uintptr_t)node;

	return 0;
}

void *
frag_stab(struct frag *fg, size_t pos)
{
	struct frag_node *match;

	if (!fg->cur) return 0;

	match = frag_find(fg, pos);
	if (!in_range(match->off, match->len, pos - fg->dsp)) {
		return 0;
	}

	return match;
}
