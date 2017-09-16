#include <errno.h>
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

static int bal(uintptr_t tag);
static struct frag_node *frag_find(struct frag *, size_t pos);
static enum link frag_cmp(struct frag_node *node, ptrdiff_t pos); 
static void frag_balance(struct frag *fg, enum link k);

void
add_chld(uintptr_t p, uintptr_t c, enum link k)
{
	struct frag_node *pp;
	struct frag_node *cc;

	pp = untag(p);
	cc = untag(c);
	pp->link[ k] = c;
	cc->link[!k] = p;
	cc->link[up] = p;
}

void
add_link(struct frag_node *node, uintptr_t tag, enum link k)
{
	node->link[k] = tag;
}

int
bal(uintptr_t tag)
{
	return tag & 3 ? (tag & 3) * -2 + 3 : 0;
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
	struct frag_node *node = untag(fg->cur);
	uintptr_t tag = fg->cur;
	uintptr_t prev;

	if (!node) return;

	while (node->link[up]) {

		prev = tag;
		tag = node->link[up];
		node = untag(node->link[up]);

		if (node->link[right] == prev) {
			fg->dsp -= node->dsp;
		}

	}

	fg->cur = tag;
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

	if (in_range(match->off + 1, match->len - 1, node->off - fg->dsp)) {
		return EEXIST;
	}

	k = node->off - fg->dsp > match->off;

	add_chld(fg->cur, (uintptr_t)node, k);

	if (k) {
		match->wid += node->len;
		fg->dsp += match->dsp;
	} else {
		match->off += node->len;
		match->dsp += node->len;
	}

	node->off = 0;
	fg->cur = (uintptr_t)node;

	frag_balance(fg, k);

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

void
frag_balance(struct frag *fg, enum link k)
{
	struct frag_node *node;
	uintptr_t next;

	for (node = untag(fg->cur);
	     node->link[up];
	     node = untag(next)) {

		next = node->link[up];

		switch (bal(next)) {
		case -1:
			if (k) __builtin_trap();
			node->link[up] ^= 2;
			break;

		case  1:
			if (!k) __builtin_trap();
			node->link[up] ^= 1;
			break;

		case  0:
			node->link[up] |= k ? 1 : 2;
			break;
		}
	}
}
