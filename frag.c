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

static void add_chld(uintptr_t p, uintptr_t c, enum link k);
static int bal(uintptr_t tag);
static void init_node(struct frag_node *node, size_t pos);
static void rebalance(struct frag *fg, enum link n);
static uintptr_t rotate(uintptr_t, enum link k);
//static uintptr_t rotate2(uintptr_t, enum link k);
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

int
adjust_balance(uintptr_t tag, enum link k)
{
	struct frag_node *node = untag(tag);
	//uintptr_t chld = node->link[k];
	int b = bal(node->link[2]);

	if (!b) {
		if (!node->link[2]) return 0;
		node->link[2] |= k ? 1 : 2;
		return k ? 1 : 2;
	}

	switch (b) {

	case -1:
		if (!k) __builtin_trap();
		node->link[2] ^= 2;
		return 2;

	case  1:
		if (k) __builtin_trap();
		node->link[2] ^= 1;
		return 1;

	}

	__builtin_unreachable();
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
rebalance(struct frag *fg, enum link n)
{
	struct frag_node *prnt;
	struct frag_node *chld;
	uintptr_t tag;
	enum link k=-1;
	size_t adj;
	int m=0;

	tag = fg->cur;
	chld = untag(tag);
	prnt = untag(chld->link[2]);
	adj = n ? chld->len : -chld->len;

	while (prnt) {

		k = tag == prnt->link[1];
		prnt->link[k] ^= m;

		if (k) prnt->wid += adj;
		else prnt->dsp += adj;

		m = adjust_balance(tag, k == n);

		tag = chld->link[2];
		chld = untag(chld->link[2]);
		prnt = untag(prnt->link[2]);

	}

	if (!prnt) return;

	k = tag == prnt->link[1];
	prnt->link[k] ^= m;
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

	prnt->link[!k] = heir->link[k];
	heir->link[k] = prnt_tag;

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
		if (k == 2) break;
		lshift_ptr(&prnt, &chld, untag(chld->link[k]));

		if (k) fg->dsp += prnt->dsp;

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
