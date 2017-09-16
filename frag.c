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
static void frag_balance(struct frag *fg);
static enum link frag_cmp(struct frag_node *node, ptrdiff_t pos); 
static struct frag_node *frag_find(struct frag *, size_t pos);
static struct frag_node frag_node(size_t offset, size_t length);

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

void
frag_balance(struct frag *fg)
{
	struct frag_node *node;
	struct frag_node *prnt;
	enum link k=-1;
	uintptr_t next;
	int m=0;

	prnt = untag(fg->cur);
	while (prnt->link[up]) {

		node = prnt;
		next = node->link[up];
		prnt = untag(next);

		k = node == untag(prnt->link[right]);
		prnt->link[k] ^= m;

		switch (bal(next)) {
		case -1:
			if (!k) __builtin_trap();
			node->link[up] ^= 2;
			m = 2;
			goto done;

		case  1:
			if (k) __builtin_trap();
			node->link[up] ^= 1;
			m = 1;
			goto done;

		case  0:
			node->link[up] |= k ? 1 : 2;
			m = k ? 1 : 2;
			break;
		}

	}

	return;
 done:
	node = untag(prnt->link[up]);
	k = prnt == untag(node->link[right]);
	node->link[k] ^= m;
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
		if (!next) return 0;

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
	struct frag_node *prnt;
	enum link k;

	if (!fg->cur) {
		*node = frag_node(node->off, node->len);
		fg->cur = (uintptr_t)node;

		return 0;
	}

	if (frag_find(fg, node->off) && node->off - fg->dsp) {
		return EEXIST;
	}

	prnt = untag(fg->cur);
	k = node->off - fg->dsp > prnt->off;

	add_chld(fg->cur, (uintptr_t)node, k);

	if (k == left) {
		prnt->off += node->len;
		prnt->dsp += node->len;
	} else {
		prnt->wid += node->len;
		fg->dsp += prnt->dsp;
	}

	node->off = node->off - fg->dsp;
	fg->cur = (uintptr_t)node;

	frag_balance(fg);

	return 0;
}

struct frag_node
frag_node(size_t offset, size_t length)
{
	return (struct frag_node) {
		.off=offset,
		.len=length,
		.dsp=length,
	};
}

void *
frag_stab(struct frag *fg, size_t pos)
{
	if (!fg->cur) return 0;
	return frag_find(fg, pos);
}
