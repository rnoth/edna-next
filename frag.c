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

static void add_chld(uintptr_t p, uintptr_t c, enum link k);
static void init_node(struct frag_node *node, size_t pos);
static uintptr_t get_chld(struct frag_node *n, enum link k);
static void rebalance(struct frag *fg, enum link n);
static uintptr_t rotate(uintptr_t, enum link k);
//static uintptr_t rotate2(uintptr_t, enum link k);
static enum link frag_cmp(struct frag *fg, size_t pos);
static void frag_step(struct frag *fg, enum link k);

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
adjust(struct frag_node *node, int d)
{
	struct frag_node *prnt;
	struct frag_node *chld;
	enum link k;
	int b;

	if (node->link[2]) {

		prnt = untag(node->link[2]);
		k = node == untag(prnt->link[1]);
		b = tag_of(prnt->link[k]);

		if (!b) b = 2|d;
		else b = 0;

		prnt->link[k] = (uintptr_t)node + b;

	}

	if (get_chld(node, 0)) {

		chld = untag(node->link[0]);

		b = tag_of(chld->link[2]);
		if (!b) b = 2|d;
		else b = 0;

		chld->link[2] = (uintptr_t)node | b;

		if (get_chld(chld, 1)) {
			chld = untag(chld->link[1]);
		}
		chld->link[1] = (uintptr_t)node | b;

	}
 
	if (get_chld(node, 1)) {

		chld = untag(node->link[1]);

		b = tag_of(chld->link[2]);
		if (!b) b = 2|d;
		else b = 0;
 
		chld->link[2] = (uintptr_t)node | b;

		if (get_chld(node, 0)) {
			chld = untag(chld->link[0]);
		}
 
		chld->link[0] = (uintptr_t)node | b;
	}

}

int
adjust_balance(uintptr_t cur, enum link k)
{
	struct frag_node *node = untag(cur);
	uintptr_t chld = node->link[k];
	int b = tag_of(cur);

	if (!b || (b&1) != k) {
		adjust(node, k);
		return 0;
	}

	if (b == tag_of(chld)) {
		adjust(node, !(b&1));
		adjust(untag(chld), !(b&1));
		rotate(cur, !k);
		return 1;
	}

	__builtin_trap();
}

void
init_node(struct frag_node *node, size_t pos)
{
	*node = (struct frag_node){.len = node->len};
	node->wid = 0;
	node->dsp = pos;
}

uintptr_t
get_chld(struct frag_node *n, enum link k)
{
	return untag(n->link[k]) != untag(n->link[2]) ? n->link[k] : 0;
}

void
rebalance(struct frag *fg, enum link n)
{
	struct frag_node *prnt;
	struct frag_node *chld;
	uintptr_t tag;
	enum link k=-1;
	size_t adj;

	tag = fg->cur;
	chld = untag(fg->cur);
	prnt = untag(chld->link[2]);
	adj = n ? chld->len : -chld->len;

	while (prnt) {

		k = tag == prnt->link[1];

		if (k) prnt->wid += adj;
		else prnt->dsp += adj;

		if (adjust_balance(chld->link[2], k == n)) break;
		tag = chld->link[2];

		chld = untag(chld->link[2]);
		prnt = untag(prnt->link[2]);

	}
}

uintptr_t
rotate(uintptr_t prnt_tag, enum link k)
{
	struct frag_node *prnt;
	struct frag_node *heir;
	uintptr_t heir_tag;

	prnt = untag(prnt_tag);
	heir_tag = prnt->link[!k];
	heir = untag(heir_tag);

	prnt->link[!k] = get_chld(heir, k) ? heir->link[k] : heir_tag;
	heir->link[k] = heir->link[2];

	heir->link[2] = prnt->link[2];
	prnt->link[2] = heir_tag;

	return heir_tag;

}

uintptr_t
rotate2(uintptr_t tag, enum link k)
{
	struct frag_node *prnt;

	prnt = untag(tag);
	prnt->link[!k] = rotate(prnt->link[!k], !k);
	return rotate(tag, k);
}

enum link
frag_cmp(struct frag *fg, size_t pos)
{
	struct frag_node *node = untag(fg->cur);

	pos -= fg->off;

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
	struct frag_node *prnt;
	enum link k;

	match = frag_stab(fg, pos);
	if (!match) return;

	rebalance(fg, 0);

	fg->cur = match->link[up];
	prnt = untag(match->link[up]);
	if (!prnt) return;
	k = untag(prnt->link[1]) == match;
	prnt->link[k] = 0;

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

		if (node->link[0] == prev) {
			fg->rem -= node->wid;
		}

		if (node->link[1] == prev) {
			fg->off -= node->dsp;
		}

	}

	fg->cur = tag;
}

int
frag_insert(struct frag *fg, size_t where, struct frag_node *new)
{
	struct frag_node *node;
	uintptr_t pred;
	uintptr_t succ;
	size_t off;
	int k;

	if (!fg->cur) {
		init_node(new, where);
		fg->cur = (uintptr_t)new;
		return 0;
	}

	node = untag(fg->cur);
	off = where;

	while (node) {

		k = frag_cmp(fg, where);
		if (!node->link[k]) break;
		if (node->link[k] == succ || node->link[k] == pred) {
			k = where - fg->off > node->dsp; // ?
			break;
		}
		if (k == 0) succ = fg->cur;
		if (k == 1) pred = fg->cur;
		frag_step(fg, k);
		node = untag(fg->cur);

	}

	add_chld(fg->cur, (uintptr_t)new, k);
	fg->cur = (uintptr_t)new;

	rebalance(fg, 1);

	return 0;
}

void *
frag_stab(struct frag *fg, size_t pos)
{
	struct frag_node *node;
	enum link k;

	if (!fg->cur) return 0;

	node = untag(fg->cur);

	while (!in_range(node->dsp, node->len, pos - fg->off)) {

		k = frag_cmp(fg, pos);
		if (!node->link[k]) return 0;
		frag_step(fg, k);
		node = untag(fg->cur);

	}

	return node;
}

void
frag_step(struct frag *fg, enum link k)
{
	struct frag_node *prev = untag(fg->cur);
	struct frag_node *next = untag(prev->link[k]);

	fg->cur = prev->link[k];
	
	if (k == 0) {
		fg->rem += prev->wid;
		return;
	} else if (k == 1) {
		fg->off += prev->dsp;
		return;
	}

	if (untag(next->link[0]) == prev) {
		fg->rem -= next->wid;
		return;
	} else {
		fg->off -= next->dsp;
		return;
	}
}
