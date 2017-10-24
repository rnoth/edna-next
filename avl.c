#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include <avl.h>
#include <tag.h>
#include <util.h>

#define foreach_ancestor(P, K)    \
	for(uintptr_t*_p=&(P),_g; \
	    *_p;                  \
	    K=(_g=avl_prnt_of(*_p))?avl_branch_of(*_p,_g):0,*_p=_g)

void
avl_add_chld(uintptr_t p, int k, uintptr_t c)
{
	struct avl *P=untag(p), *C=untag(c);

	P->link[k] = c, C->link[2] = p;
}

uintptr_t
avl_adjust(uintptr_t t, int b)
{
	uintptr_t c, p, u = t & ~3 | b;

	if (c = avl_chld_of(t, 0)) avl_link_to(c, 2, u);
	if (c = avl_chld_of(t, 1)) avl_link_to(c, 2, u);
	if (p = avl_prnt_of(t)) avl_link_to(p, avl_branch_of(t, p), u);

	return u;
}

int
avl_branch_of(uintptr_t t, uintptr_t p)
{
	return t == avl_chld_of(p, 1);
}

uintptr_t
avl_detatch(uintptr_t u, int k)
{
	struct avl *U = untag(u);
	struct avl *V = untag(U->link[k]);
	uintptr_t r;

	if (!V) return 0;

	V->link[2] = 0;

	r = U->link[k];
	U->link[k] = 0;

	return r;
}

uintptr_t
avl_increment(uintptr_t t, int k)
{
	uintptr_t u = avl_chld_of(t, k);
	uintptr_t v = u ? avl_chld_of(u, !k) : 0;
	uintptr_t p = avl_prnt_of(t);
	int b = tag_of(t), d = tag_of(v);
	int l = p ? avl_branch_of(t, p) : 0;

	if (!b || b-2 != k) return avl_adjust(t, b ? 0 : 2|k);

	if (b == tag_of(u)) {
		avl_adjust(t, d ? d ^ 1 : 0);
		avl_adjust(u, 0);
		t = avl_rotate(t & ~3, !k);
		if (p) avl_add_chld(p, l, t);
		return t;
	}

	avl_adjust(t, 0);
	avl_adjust(u, d);
	t = avl_rotate2(t ^ b, !k);
	if (p) avl_add_chld(p, l, t);
	return t;
}

uintptr_t
avl_tag_of(struct avl *F)
{
	uintptr_t f=tag0(F);

	if (!F->link[0] && !F->link[1]) return f;

	if (!F->link[0]) return f|3;
	if (!F->link[1]) return f|2;

	return avl_prnt_of(F->link[0]);
}

void
avl_delete(struct avl *T)
{
	uintptr_t t, p, q;
	int k;

	if (!T) return;

	t = avl_tag_of(T);

	if (avl_chld_of(t, 1)) avl_swap(t, 1);
	
	p = q = avl_prnt_of(t);
	if (!p) return;

	k = avl_branch_of(t, p);
	avl_detatch(p, k);

	foreach_ancestor (q, k) {
		q = avl_increment(q, !k);
		if (!tag_of(q)) break;
	}
}

void *
avl_get_root(struct avl *T)
{
	uintptr_t x;

	if (!T) return 0;
	while (x=T->link[2]) T=untag(x);

	return T;
}

void
avl_free(struct avl *T)
{
	uintptr_t t, p, c0, c1;

	if (!T) return;
	t = avl_tag_of(T);

 again:
	p = avl_prnt_of(t);
	while ((c0 = avl_chld_of(t, 0)) || (c1 = avl_chld_of(t, 1))) {
		p = t; if (c0) t = c0; else t = c1;
	}

	free(untag(t));
	if (p){
		avl_link_to(p, avl_branch_of(t, p), 0);
		t = p;
		goto again;
	}
}

void
avl_insert(struct avl *H, int k, struct avl *F)
{
	uintptr_t p, t=tag0(F);

	if (!H) return;

	p = avl_tag_of(H);
	avl_add_chld(p, k, t);

	foreach_ancestor (p, k) {
		p = avl_increment(p, k);
		if (!tag_of(p)) break;
	}
}

uintptr_t
avl_next(uintptr_t t, int k)
{
	uintptr_t u, x;

	u = avl_chld_of(t, k);

	if (u) {
		for (; x = avl_chld_of(u, !k); u = x);
		return u;
	}

	for (; (x = avl_prnt_of(t)) && k == avl_branch_of(t, x); t = x);
	if (!x) return 0;
	return x;
}

uintptr_t
avl_rotate(uintptr_t p, int k)
{
	uintptr_t h = avl_detatch(p, !k);
	uintptr_t v = avl_detatch(h, k);

	if (v) avl_add_chld(p, !k, v);
	avl_add_chld(h, k, p);

	return h;
}

uintptr_t
avl_rotate2(uintptr_t t, int k)
{
	uintptr_t c;

	c = avl_detatch(t, !k);
	c = avl_rotate(c, !k);
	avl_add_chld(t, !k, c);

	return avl_rotate(t, k);
}

uintptr_t
avl_swap(uintptr_t u, int k)
{
	uintptr_t x;
	uintptr_t p, q;
	uintptr_t c, v;

	x = avl_next(u, k), q = avl_prnt_of(x), v = avl_detatch(x, k);

	avl_detatch(q, avl_branch_of(x, q));

	if (p = avl_prnt_of(u)) avl_add_chld(p, avl_branch_of(u, p), x);

	if (q != u) avl_add_chld(q, !k, u);
	else avl_detatch(u, k), avl_add_chld(x, k, u);

	if (c = avl_detatch(u, 0)) avl_add_chld(x, 0, c);
	if (c = avl_detatch(u, 1)) avl_add_chld(x, 1, c);
	if (v) avl_add_chld(u, k, v);

	return x ^ tag_of(u);
}
