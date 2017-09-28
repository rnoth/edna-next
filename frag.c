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
static uintptr_t detatch(uintptr_t p, int k);
static void increment_chld(uintptr_t u, int k);
static void init_node(struct frag_node *d, size_t x);
static uintptr_t get_chld(uintptr_t n, int k);
static uintptr_t get_prnt(uintptr_t prnt);
static int find_leftmost_leaf(struct frag *fg);
static int find_empty_chld(struct frag *fg, size_t where);
static void init_node(struct frag_node *node, size_t pos);
static int frag_cmp(struct frag *fg, size_t pos);
static void frag_step(struct frag *fg, int k);
static void rebalance(struct frag_node *g, int r);
static uintptr_t rotate(uintptr_t, int k);
static uintptr_t rotate2(uintptr_t, int k);
static void set_link(uintptr_t u, int k, uintptr_t t);
static void set_off(uintptr_t u, int b, size_t n);
static uintptr_t swap(uintptr_t u, int k);

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
detatch(uintptr_t u, int k)
{
	struct frag_node *U = untag(u);
	struct frag_node *V = untag(U->link[k]);
	uintptr_t r = U->link[k];

	if (!V) return 0;

	if (V->link[!k] == V->link[2]) V->link[!k] = 0;
	V->link[2] = 0;

	r = U->link[k];
	U->link[k] = 0;

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
init_node(struct frag_node *d, size_t x)
{
	d[0] = (struct frag_node){.len = d->len};
	d->off[1] = 0;
	d->off[0] = x;
}

bool
is_leaf(uintptr_t t)
{
	return !get_chld(t, 0) || !get_chld(t, 1);
}

uintptr_t
get_chld(uintptr_t u, int k)
{
	struct frag_node *g = untag(u);
	if (g->link[k] == g->link[2]) return 0x0;
	return g->link[k];
}

size_t
get_off(uintptr_t u, int b)
{
	struct frag_node *g = untag(u);
	return g->off[b];
}

uintptr_t
get_next(uintptr_t u, int k)
{
	uintptr_t c;

	u = get_chld(u, k);
	if (!u) return 0x0;

	c = get_chld(u, !k);
	return c ? c : u;
}

uintptr_t
get_prnt(uintptr_t u)
{
	struct frag_node *g;
	g = untag(u);
	return g->link[2];
}

uintptr_t
get_tag(struct frag_node *g)
{
	uintptr_t u=tag0(g);

	if (get_chld(u, 0)) {
		u = get_prnt(g->link[0]);
	} else if (get_chld(u, 1)) {
		u = get_prnt(g->link[1]);
	} else {
		u = 0x0;
	}

	return tag_of(u);
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
find_empty_chld(struct frag *fg, size_t where)
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
frag_delete(struct frag *f, struct frag_node *T)
{
	uintptr_t t, p;
	int k;

	if (!T) return;

	t = tag0(T) + get_tag(T);

	if (!is_leaf(t)) f->cur = swap(t, 1);

	rebalance(T, 0);

	if (f->cur == t) {
		if (get_prnt(t)) frag_step(f, 2);
		else if (get_chld(t, 1)) f->cur = get_chld(t, 1);
		else f->cur = get_chld(t, 0);
	}

	p = T->link[2];
	if (!p) return;

	k = get_chld(p, 1) == t;
	detatch(p, k);
}

void
frag_flush(struct frag *fg)
{
	if (!fg->cur) return;

	while (get_prnt(fg->cur)) frag_step(fg, 2);
}

int
frag_insert(struct frag *f, size_t x, struct frag_node *g)
{
	uintptr_t u;
	int k;

	if (!f) return EINVAL;
	if (!g) return EINVAL;

	u = tag0(g);

	if (!f->cur) {
		init_node(g, x);
		f->cur = u;
		return 0;
	}

	k = find_empty_chld(f, x);

	set_link(f->cur, k, u);
	g->link[2] = g->link[!k] = f->cur;
	f->cur = u;

	rebalance(g, 1);

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

	fg->cur = prev->link[k];
	
	if (!next) return;

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
rebalance(struct frag_node *g, int r)
{
	struct frag_node *G = g;
	size_t a;
	int k;

	a = r ? g->len : -g->len;

	while (G = untag(G->link[2])) {

		k = g == untag(G->link[1]);

		if (k) G->off[1] += a;
		else G->off[0] += a;

		increment_chld(g->link[2], k == r);

		if (!tag_of(g->link[2])) return;
		g = untag(g->link[2]);

	}
}

uintptr_t
rotate(uintptr_t p, int k)
{
	uintptr_t h;
	uintptr_t v;

	h = detatch(p, !k);
	v = detatch(h, k);

	if (v) add_chld(p, !k, v);
	else set_link(p, !k, h);
	add_chld(h, k, p);

	return h;
}

uintptr_t
rotate2(uintptr_t t, int k)
{
	uintptr_t c;

	c = detatch(t, !k);
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

uintptr_t
swap(uintptr_t u, int k)
{
	struct frag_node *U=untag(u);
	struct frag_node *X;
	uintptr_t x;
	
	x = get_next(u, k);
	X = untag(x);

	if (X->link[2] == u) X->link[2] = x;
	memswp(X->link+2, U->link+2, sizeof U->link[2]);

	if (U->link[k] == x) U->link[k] = u;
	memswp(X->link+k, U->link+k, sizeof U->link[k]);

	if (X->link[!k] == u) X->link[!k] = x;
	memswp(X->link+!k, U->link+!k, sizeof U->link[!k]);

	memswp(U->off, X->off, sizeof U->off);
	X->off[k] -= X->len, X->off[k] += U->len;

	return x;
}
