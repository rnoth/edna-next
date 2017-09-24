#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include <frag.h>
#include <util.h>
#include <tag.h>

static void add_chld(uintptr_t p, int k, uintptr_t c);
static void adjust_balance(uintptr_t u, int d);
static void adjust_by_chld(uintptr_t u, int k, int d);
static void adjust_by_prnt(uintptr_t u, int d);
static void increment_chld(uintptr_t u, int k);
static void init_node(struct frag_node *node, size_t pos);
static uintptr_t get_chld(uintptr_t n, int k);
static uintptr_t get_prnt(uintptr_t prnt);
static int find_leftmost_leaf(struct frag *fg);
static int find_nearest_leaf(struct frag *fg, size_t where);
static void init_node(struct frag_node *node, size_t pos);
static int frag_cmp(struct frag *fg, size_t pos);
static void frag_step(struct frag *fg, int k);
static void rebalance(uintptr_t cur, int r);
static uintptr_t rotate(uintptr_t, int k);
static uintptr_t rotate2(uintptr_t, int k);
static void set_link(uintptr_t u, int k, uintptr_t t);
static void set_off(uintptr_t u, int b, size_t n);

void
add_chld(uintptr_t p, int k, uintptr_t c)
{
	struct frag_node *pp;
	struct frag_node *cc;

	assert(k == 0 || k == 1);
	assert(p > 0x3);
	assert(c > 0x3);

	pp = untag(p);
	cc = untag(c);

	if (!cc->link[!k]) cc->link[!k] = p;

	pp->link[k] = c;
	cc->link[2] = p;

	pp->off[k] = cc->off[0] + cc->off[1] + cc->len;
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
		if (!get_chld(c, !k)) set_link(c, !k, t);
		set_link(c, 2, t);
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

uintptr_t
detatch_chld(uintptr_t u, int k)
{
	struct frag_node *g = untag(u);
	struct frag_node *h = untag(g->link[k]);
	uintptr_t r = g->link[k];

	if (!h) return 0;

	if (h->link[!k] == h->link[2]) h->link[!k] = 0;
	h->link[2] = 0;

	r = g->link[k];
	g->link[k] = 0;

	set_off(u, k, 0);
	return r;
}

void
increment_chld(uintptr_t u, int k)
{
	uintptr_t c = get_chld(u, k);
	uintptr_t g = c ? get_chld(c, !k) : 0;
	int w = tag_of(g);
	int b = tag_of(u);

	if (!b || b-2 != k) {
		adjust_balance(u, b ? 0 : 2|k);
		return;
	}

	if (b == tag_of(c)) {
		adjust_balance(u, w ? w ^ 1 : 0);
		adjust_balance(c, 0);
		rotate(u & ~3, !k);
		return;
	}

	adjust_balance(u, 0);
	adjust_balance(c, w);
	rotate2(u ^ b, !k);
}

void
init_node(struct frag_node *node, size_t pos)
{
	*node = (struct frag_node){.len = node->len};
	node->off[1] = 0;
	node->off[0] = pos;
}

uintptr_t
get_chld(uintptr_t u, int k)
{
	struct frag_node *n = untag(u);
	if (n->link[k] == n->link[2]) return 0x0;
	return n->link[k];
}

size_t
get_off(uintptr_t u, int b)
{
	struct frag_node *d = untag(u);
	return d->off[b];
}

uintptr_t
get_prnt(uintptr_t t)
{
	struct frag_node *c;
	c = untag(t);
	return c->link[2];
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
			return where - fg->off > node->off[0];
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

int
frag_cmp(struct frag *fg, size_t pos)
{
	struct frag_node *node = untag(fg->cur);

	pos -= fg->off;
	if (pos <= node->off[0]) {
		return 0;
	}

	pos -= node->off[0];
	if (pos < node->off[1]) {
		return 1;
	}

	return 2;

}

void
frag_delete(struct frag *fg, size_t pos)
{
	struct frag_node *match;
	struct frag_node *prnt;
	int k;

	match = frag_stab(fg, pos);
	if (!match) return;

	rebalance(fg->cur, 0);

	fg->cur = match->link[2];
	prnt = untag(match->link[2]);
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

	set_link(fg->cur, k, tag0(new));
	new->link[2] = fg->cur;
	new->link[!k] = fg->cur;
	fg->cur = tag0(new);

	rebalance(fg->cur, 1);

	return 0;
}

void *
frag_stab(struct frag *fg, size_t pos)
{
	struct frag_node *node;
	int k;

	if (!fg) return 0;
	if (!fg->cur) return 0;

	node = untag(fg->cur);

	while (!in_range(node->off[0], node->len, pos - fg->off)) {

		k = frag_cmp(fg, pos);
		if (!node->link[k]) return 0;
		frag_step(fg, k);
		node = untag(fg->cur);

	}

	return node;
}

void
frag_step(struct frag *fg, int k)
{
	struct frag_node *prev = untag(fg->cur);
	struct frag_node *next = untag(prev->link[k]);

	assert(fg->cur > 0x3);
	assert(prev->link[k] > 0x3);

	fg->cur = prev->link[k];
	
	if (k == 0) {
		fg->rem += prev->off[1] + prev->len;
		return;
	} else if (k == 1) {
		fg->off += prev->off[0] + prev->len;
		return;
	}

	if (untag(next->link[0]) == prev) {
		fg->rem -= next->off[1];
		return;
	} else {
		fg->off -= next->off[0];
		return;
	}
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

		if (k) n->off[1] += a;
		else n->off[0] += a;

		increment_chld(get_prnt(u), k == r);

		u = get_prnt(u);
		if (!tag_of(u)) return;

	}
}

uintptr_t
rotate(uintptr_t p, int k)
{
	uintptr_t h;
	uintptr_t v;

	h = detatch_chld(p, !k);
	v = detatch_chld(h, k);

	if (v) add_chld(p, !k, v);
	else set_link(p, !k, h);
	add_chld(h, k, p);

	return h;
}

uintptr_t
rotate2(uintptr_t t, int k)
{
	uintptr_t c;

	c = detatch_chld(t, !k);
	c = rotate(c, !k);
	add_chld(t, !k, c);

	t = rotate(t, k);

	return t;
}

void
set_link(uintptr_t u, int k, uintptr_t t)
{
	struct frag_node *n = untag(u);
	n->link[k] = t;
}

void
set_off(uintptr_t u, int b, size_t n)
{
	struct frag_node *d = untag(u);
	d->off[b] = n;
}
