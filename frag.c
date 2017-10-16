#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include <frag.h>
#include <util.h>
#include <tag.h>

static inline uintptr_t get_chld(uintptr_t t, int k);
static inline size_t    get_end(uintptr_t t);
static inline uintptr_t get_len(uintptr_t t);

static inline uintptr_t get_link(uintptr_t t, int k);
static inline size_t    get_off(uintptr_t t);
static inline size_t    get_max(uintptr_t t);

static inline uintptr_t get_prnt(uintptr_t t);

static void      add_chld(uintptr_t p, int k, uintptr_t c);
static uintptr_t adjust(uintptr_t t, int b);
static int       branch_of(uintptr_t t, uintptr_t p);

static int       cmp(uintptr_t t, size_t p);
static uintptr_t detatch(uintptr_t p, int k);
static uintptr_t increment(uintptr_t t, int k);

static void      init(struct frag *d, size_t p);
static int       find_rightmost_leaf(uintptr_t *t);
static int       find_empty_chld(uintptr_t *t, size_t p);

static uintptr_t get_next(uintptr_t t, int k, size_t *);
static uintptr_t get_tag(struct frag *F);
static void      offset(uintptr_t p, size_t n);

static uintptr_t rotate(uintptr_t, int k);
static uintptr_t rotate2(uintptr_t, int k);

static void      set_link(uintptr_t u, int k, uintptr_t t);
static void      set_max(uintptr_t t);
static size_t    step(uintptr_t t, int k);

static uintptr_t swap(uintptr_t u, int k);
static size_t    try_max(uintptr_t t, size_t m);

#define get_field(P, F) (((struct frag *)untag(P))->F)

#define foreach_ancestor(P, K) \
	for(uintptr_t*_p=&P,_g;*_p;K=(_g=get_prnt(*_p))?branch_of(*_p,_g):0,*_p=_g)

uintptr_t get_chld(uintptr_t t, int k) { return get_field(t, link[k]); }
size_t get_end(uintptr_t t) { return get_off(t) + get_len(t); }
uintptr_t get_len(uintptr_t t) { return get_field(t, len); }
uintptr_t get_link(uintptr_t t, int k) { return get_field(t, link[k]); }
size_t get_off(uintptr_t t) { return get_field(t, off); }
size_t get_max(uintptr_t t) { return get_field(t, max); }
uintptr_t get_prnt(uintptr_t t) { return get_field(t, link[2]); }

void
add_chld(uintptr_t p, int k, uintptr_t c)
{
	struct frag *P=untag(p), *C=untag(c);

	P->link[k] = c, C->link[2] = p;
	set_max(p);
}

uintptr_t
adjust(uintptr_t t, int b)
{
	uintptr_t c, p, u = t & ~3 | b;

	if (c = get_chld(t, 0)) set_link(c, 2, u);
	if (c = get_chld(t, 1)) set_link(c, 2, u);
	if (p = get_prnt(t)) set_link(p, branch_of(t, p), u);

	return u;
}

int
branch_of(uintptr_t t, uintptr_t p)
{
	return t == get_chld(p, 1);
}

int
cmp(uintptr_t t, size_t p)
{
	if (p <= get_off(t)) return 0;
	if (p < get_max(t)) return 1; // XXX
	return 2;
}

uintptr_t
detatch(uintptr_t u, int k)
{
	struct frag *U = untag(u);
	struct frag *V = untag(U->link[k]);
	uintptr_t r;

	if (!V) return 0;

	V->link[2] = 0;

	r = U->link[k];
	U->link[k] = 0;
	set_max(u);

	return r;
}

void
inc_max(uintptr_t t, size_t n)
{
	get_field(t, max) += n;
}

void
inc_off(uintptr_t t, size_t n)
{
	get_field(t, off) += n;
}

uintptr_t
increment(uintptr_t t, int k)
{
	uintptr_t u = get_chld(t, k);
	uintptr_t v = u ? get_chld(u, !k) : 0;
	int b = tag_of(t), d = tag_of(v);

	if (!b || b-2 != k) return adjust(t, b ? 0 : 2|k);

	if (b == tag_of(u)) {
		adjust(t, d ? d ^ 1 : 0);
		adjust(u, 0);
		return rotate(t & ~3, !k);
	}

	adjust(t, 0);
	adjust(u, d);
	return rotate2(t ^ b, !k);
}

void
init(struct frag *n, size_t x)
{
	n[0] = (struct frag){.len = n->len};
	n->off = x;
	n->max = n->len;
}

bool
is_leaf(uintptr_t t)
{
	return !get_chld(t, 0) || !get_chld(t, 1);
}

uintptr_t
get_next(uintptr_t t, int k, size_t *f)
{
	*f += k ? get_off(t) : 0, t = get_chld(t, k);
	for (uintptr_t x; x = get_chld(t, !k); t = x) {
		*f += !k ? get_off(t) : 0;
	}
	return t;
}

uintptr_t
get_tag(struct frag *F)
{
	uintptr_t f=tag0(F);

	if (!F->link[0] && !F->link[1]) return f;

	if (!F->link[0]) return f|3;
	if (!F->link[1]) return f|2;

	return get_prnt(F->link[0]);
}

int
find_empty_chld(uintptr_t *t, size_t p)
{
	uintptr_t x;
	int k;

	while (k = cmp(*t, p), x = get_link(*t, k)) {
		p -= step(*t, k), *t = x;
	}

	if (k == 2) return find_rightmost_leaf(t);

	return k;
}

int
find_rightmost_leaf(uintptr_t *t)
{
	uintptr_t x;
	while (x = get_chld(*t, 1)) *t = x;
	return 1;
}

void
frag_append(struct frag *H, size_t n, struct frag *F)
{
	__builtin_trap();
}

void
frag_delete(struct frag *T)
{
	uintptr_t t, p, q;
	int k;

	if (!T) return;

	t = get_tag(T);

	if (!is_leaf(t)) swap(t, 1);
	
	p = q = get_prnt(t);
	if (!p) return;

	k = branch_of(t, p);
	detatch(p, k);

	foreach_ancestor (q, k) {
		set_max(q);
		//q = increment(q, !k);
		if (!tag_of(q)) break;
	}
}

void *
frag_get_root(struct frag *T)
{
	uintptr_t x;

	if (!T) return 0;
	while (x=T->link[2]) T=untag(x);

	return T;
}

void
frag_insert(struct frag *H, size_t n, struct frag *F)
{
	uintptr_t p, t=tag0(F);
	size_t m;
	int k;

	if (!H) { init(F, n); return; }

	p = get_tag(H);
	k = find_empty_chld(&p, n);

	set_link(p, k, t);
	set_link(t, 2, p);

	if (k) F->off = get_len(p);
	m = F->off + F->len;

	foreach_ancestor (p, k) {
		if (!k) offset(p, F->len);
		else if (m) m = try_max(p, m);

		p = increment(p, k);
		if (!tag_of(p)) break;
	}
}

void
frag_offset(struct frag *T, size_t f)
{
	uintptr_t t=get_tag(T);
	int k=0;

	foreach_ancestor (t, k) {
		if (k) return;
		inc_off(t, f);
	}
}

void *
frag_stab(struct frag *H, size_t p)
{
	uintptr_t h, x, d=0;
	int k;

	if (!H) return 0;

	h = get_tag(H);

	while (!in_range(get_off(h), get_len(h), p)) {
		k = cmp(h, p);
		p+= step(h, k);
		x = get_link(h, k);
		if (!x || x == d) return 0x0;
		d = h, h = x;
	}

	return untag(h);
}

void
offset(uintptr_t p, size_t n)
{
	inc_off(p, n);
	inc_max(p, n);
}

void
set_max(uintptr_t t)
{
	uintptr_t c;
	size_t m[3]={0};
	size_t r;

	m[2] = get_end(t);
	if (c=get_chld(t,0)) m[0] = get_max(c);
	if (c=get_chld(t,1)) m[1] = get_off(t) + get_max(c);

	r = umax(m[0], umax(m[1], m[2]));
	get_field(t, max) = r;
}

uintptr_t
rotate(uintptr_t p, int k)
{
	uintptr_t h = detatch(p, !k);
	uintptr_t v = detatch(h, k);

	if (!k) {
		inc_off(h, get_off(p));
	} else {
		inc_off(p, -get_off(h));
	}

	if (v) add_chld(p, !k, v);
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

	return rotate(t, k);
}

void
set_link(uintptr_t u, int k, uintptr_t t)
{
	struct frag *n = untag(u);
	n->link[k] = t;
}

size_t
step(uintptr_t t, int k)
{
	uintptr_t x;

	if (k == 0) return 0;
	if (k == 1) return get_off(t);

	x = get_prnt(t);
	if (!x) return 0;

	if (branch_of(t, x)) return -get_off(x);

	return 0;
}

uintptr_t
swap(uintptr_t u, int k)
{
	uintptr_t x;
	uintptr_t p, q;
	uintptr_t c, v;
	size_t f=0;

	x = get_next(u, k, &f), q = get_prnt(x), v = detatch(x, k);

	detatch(q, branch_of(x, q));

	if (p = get_prnt(u)) add_chld(p, branch_of(u, p), x);

	if (q != u) add_chld(q, !k, u);
	else detatch(u, k), add_chld(x, k, u);

	if (c = detatch(u, 0)) add_chld(x, 0, c);
	if (c = detatch(u, 1)) add_chld(x, 1, c);
	if (v) add_chld(u, k, v);

	inc_off(x, f);
	set_max(x);

	return x ^ tag_of(u);
}

size_t
try_max(uintptr_t t, size_t n)
{
	size_t h=get_max(t);
	size_t r=get_off(t) + n;

	if (h < r) get_field(t, max) = r;
	else return 0;

	return r;
}
