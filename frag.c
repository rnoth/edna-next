#include <assert.h>
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
static void adjust_balance(uintptr_t u, int d);
static void adjust_by_chld(uintptr_t u, int k, int d);
static void adjust_by_prnt(uintptr_t u, int d);
static void increment_chld(uintptr_t u, int k);
static void init_node(struct frag_node *node, size_t pos);
static uintptr_t get_chld(uintptr_t n, enum link k);
static uintptr_t get_prnt(uintptr_t prnt);
static int find_leftmost_leaf(struct frag *fg);
static int find_nearest_leaf(struct frag *fg, size_t where);
static void init_node(struct frag_node *node, size_t pos);
static void rebalance(uintptr_t cur, int r);
static uintptr_t rotate(uintptr_t, enum link k);
//static uintptr_t rotate2(uintptr_t, enum link k);
static enum link frag_cmp(struct frag *fg, size_t pos);
static void frag_step(struct frag *fg, enum link k);
static void set_link(uintptr_t cur, int, uintptr_t tag);

void
add_chld(uintptr_t p, uintptr_t c, enum link k)
{
	struct frag_node *pp;
	struct frag_node *cc;

	assert(k == 0 || k == 1);
	assert(p > 0x3);
	assert(c > 0x3);

	pp = untag(p);
	cc = untag(c);

	pp->link[k] = c;
	cc->link[!k] = p;
	cc->link[2] = p;

}

void
adjust_balance(uintptr_t u, int g)
{
	assert(u > 0x3);
	assert(g == 0 || g == 2 || g == 3);

	adjust_by_prnt(u, g);
	adjust_by_chld(u, 0, g);
	adjust_by_chld(u, 1, g);

}

void
adjust_by_chld(uintptr_t u, int k, int g)
{
	uintptr_t c = get_chld(u, k);
	uintptr_t t = u & ~3 | g;

	if (c) {
		set_link(c, 2, t);
		if (!get_chld(c, !k)) set_link(c, !k, t);
	}
}

void
adjust_by_prnt(uintptr_t u, int g)
{
	uintptr_t p = get_prnt(u);
	uintptr_t t = u & ~3 | g;
	int k;

	if (p) {
		k = u == get_chld(p, 1);
		set_link(p, k, t);
	}
}

void
increment_chld(uintptr_t cur, int k)
{
	uintptr_t chld = get_chld(cur, k);
	int b = tag_of(cur);

	assert(cur != 0x0);
	assert(k == 0 || k == 1);
	assert(b == 0 || b == 2 || b == 3);

	if (!b || b-2 != k) {
		adjust_balance(cur, b ? 0 : 2|k);
		return;
	}

	if (b == tag_of(chld)) {
		adjust_balance(cur, 0);
		adjust_balance(chld, 0);
		rotate(cur & ~3, !k);
		return;
	}

	__builtin_trap();
}

void
init_node(struct frag_node *node, size_t pos)
{
	assert(node != 0x0);

	*node = (struct frag_node){.len = node->len};
	node->wid = 0;
	node->dsp = pos;
}

uintptr_t
get_chld(uintptr_t n, enum link k)
{
	struct frag_node *nn = untag(n);

	assert(k == 0 || k == 1);
	assert(n != 0x0);

	return untag(nn->link[k]) != untag(nn->link[2]) ? nn->link[k] : 0;
}

uintptr_t
get_prnt(uintptr_t t)
{
	struct frag_node *c;

	assert(t > 0x3);
	c = untag(t);
	return c->link[2];
}

void
rebalance(uintptr_t u, int r)
{
	struct frag_node *n;
	size_t a;
	int k;

	n = untag(u);
	a = r ? n->len : -n->len;

	while (n = untag(n->link[2])) {

		k = u == n->link[1];

		if (k) n->wid += a;
		else n->dsp += a;

		increment_chld(get_prnt(u), k == r);

		u = get_prnt(u);
		if (!tag_of(u)) return;

	}
}

uintptr_t
rotate(uintptr_t p, enum link k)
{
	uintptr_t h;
	uintptr_t v;

	h = get_chld(p, !k);
	v = get_chld(h, k);

	set_link(p, !k, v ? v : h);
	set_link(h, k, p);

	set_link(h, 2, get_prnt(p));
	set_link(p, 2, h);

	return h;
}

uintptr_t
rotate2(uintptr_t tag, enum link k)
{
	struct frag_node *prnt;

	prnt = untag(tag);
	prnt->link[!k] = rotate(prnt->link[!k], !k);
	return rotate(tag, k);
}

int
find_leftmost_leaf(struct frag *fg)
{
	struct frag_node *node;

	while (node = untag(fg->cur)) {
		if (!node->link[1]) break;
		frag_step(fg, 1);
	}

	return 1;
}

int
find_nearest_leaf(struct frag *fg, size_t where)
{
	struct frag_node *node;
	uintptr_t pred = 0;
	uintptr_t succ = 0;
	int k;

	assert(fg != 0x0);
	assert(fg->cur > 0x3);

	while (node = untag(fg->cur)) {

		k = frag_cmp(fg, where);

		if (!node->link[k]) {
			break;
		}

		if (node->link[k] == pred || node->link[k] == succ) {
			return where - fg->off > node->dsp;
		}

		if (k == 0) succ = fg->cur;
		if (k == 1) pred = fg->cur;

		frag_step(fg, k);

	}

	if (k == 2) {
		find_leftmost_leaf(fg);
		return 1;
	}

	return k;
}

enum link
frag_cmp(struct frag *fg, size_t pos)
{
	struct frag_node *node = untag(fg->cur);

	pos -= fg->off;

	if (pos <= node->dsp) {
		return 0;
	}

	pos -= node->dsp;
	if (pos < node->len + node->wid) {
		return 1;
	}

	return 2;

}

void
frag_delete(struct frag *fg, size_t pos)
{
	struct frag_node *match;
	struct frag_node *prnt;
	enum link k;

	match = frag_stab(fg, pos);
	if (!match) return;

	rebalance(fg->cur, 0);

	fg->cur = match->link[up];
	prnt = untag(match->link[up]);
	if (!prnt) return;
	k = untag(prnt->link[1]) == match;
	prnt->link[k] = 0;

}

void
frag_flush(struct frag *fg)
{
	if (!fg->cur) return;

	while (get_prnt(fg->cur)) frag_step(fg, 2);
}

int
frag_insert(struct frag *fg, size_t where, struct frag_node *new)
{
	int k;

	if (!fg) return EINVAL;
	if (!new) return EINVAL;

	if (!fg->cur) {
		init_node(new, where);
		fg->cur = tag0(new);
		return 0;
	}

	k = find_nearest_leaf(fg, where);
	
	add_chld(fg->cur, tag0(new), k);
	fg->cur = tag0(new);

	rebalance(fg->cur, 1);

	return 0;
}

void *
frag_stab(struct frag *fg, size_t pos)
{
	struct frag_node *node;
	enum link k;

	if (!fg) return 0;
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

	assert(fg->cur > 0x3);
	assert(prev->link[k] > 0x3);

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

void
set_link(uintptr_t n, int k, uintptr_t t)
{
	struct frag_node *nn = untag(n);

	nn->link[k] = t;
}
