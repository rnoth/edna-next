#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include <frag.h>
#include <util.h>
#include <tag.h>

enum link {
	stop=-1,
	left,
	right,
	up,
};

static int bal(uintptr_t tag);
static void frag_balance(struct frag *fg);
static enum link frag_cmp(struct frag_node *node, size_t pos); 

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

void
init_node(struct frag_node *node, size_t pos)
{
	*node = (struct frag_node){.len = node->len};
	node->wid = 0;
	node->dsp = pos;
}

void
frag_balance(struct frag *fg)
{
	struct frag_node *node;
	struct frag_node *prnt;
	enum link k=-1;
	uintptr_t next;
	size_t adj;
	int m=0;

	prnt = untag(fg->cur);
	adj = prnt->len;

	while (prnt->link[up]) {

		node = prnt;
		next = node->link[up];
		prnt = untag(next);

		k = node == untag(prnt->link[right]);
		prnt->link[k] ^= m;

		if (k) prnt->wid += adj;
		else prnt->dsp += adj;

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
frag_cmp(struct frag_node *node, size_t pos)
{

	if (pos <= node->dsp) {
		return left;
	}

	pos -= node->dsp;
	if (pos < node->len + node->wid) {
		return right;
	}

	return node->link[up] ? up : right;

}

void
frag_delete(struct frag *fg, size_t pos)
{
	struct frag_node *match;

	match = frag_stab(fg, pos);
	if (!match) return;

	fg->cur = 0;
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
frag_insert(struct frag *fg, size_t where, struct frag_node *new)
{
	struct frag_node *prnt;
	struct frag_node *chld;
	size_t off;
	enum link k;

	if (!fg->cur) {
		init_node(new, where);
		fg->cur = (uintptr_t)new;
		return 0;
	}

	chld = untag(fg->cur);
	off = where;

	while (chld) {

		k = frag_cmp(chld, where - fg->dsp);
		lshift_ptr(&prnt, &chld, untag(chld->link[k]));

	}

	if (k) fg->dsp += prnt->dsp;

	add_chld(fg->cur, (uintptr_t)new, k);

	fg->cur = (uintptr_t)new;

	frag_balance(fg);

	return 0;
}

void *
frag_stab(struct frag *fg, size_t pos)
{
	struct frag_node *node;
	uintptr_t next;
	enum link k;

	if (!fg->cur) return 0;

	node = untag(fg->cur);

	while (!in_range(node->dsp, node->len, pos - fg->dsp)) {

		k = frag_cmp(node, pos - fg->dsp);

		next = node->link[k];
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
