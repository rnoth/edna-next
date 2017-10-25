#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include <txt.h>

static void text_delete_across(struct piece **ctx, size_t offset, size_t extent);
static int text_delete_within(struct piece **ctx, size_t offset, size_t extent);
static void text_relink(struct piece *, struct piece *, struct piece *);
static int text_split(struct piece **ctx, size_t offset, size_t extent);

struct piece *
text_ctor(void)
{
	struct piece *a, *z;

	a = calloc(1, sizeof *a);
	if (!a) return 0;
	z = calloc(1, sizeof *z);
	if (!z) { free(a); return 0; }

	text_link(a, z);

	return a;
}

void
text_step(struct piece **t)
{
	struct piece *x = text_next(t[0], t[1]);
	t[1] = t[0], t[0] = x;
}

int
text_delete(struct piece **t, size_t i, size_t n)
{
	if (i >= t[0]->len) i = text_walk(t, i);
	if (i + n < t[0]->len) return text_delete_within(t, i, n);
	text_delete_across(t, i, n);
	return 0;
}

void
text_delete_across(struct piece **t, size_t i, size_t n)
{
	struct piece *u[2];

	if (i) {
		n -= t[0]->len - i;
		t[0]->len = i, i = 0;
		text_step(t);
	}

	if (!t[0]->len) {
		t[0] = 0, t[0] = 0;
		return;
	}

	u[0] = t[0], u[1] = t[1];

	if (n >= t[0]->len) {
		n = text_walk(u, n);

		text_unlink(t[0], t[1]);
		text_unlink(u[0], u[1]);

		text_link(u[0], t[1]);
		u[1] = t[1];
	} else t[0] = 0;
	t[1] = 0;

	if (n) {
		u[0]->len -= n;
		u[0]->off += n;
	}

	text_merge(u);
}

int
text_delete_within(struct piece **t, size_t i, size_t n)
{
	int e = text_split(t, i, n);
	if (e) return e;

	t[0] = 0, t[1] = 0;
	return 0;
}

void
text_dtor(struct piece *t)
{
	struct piece *v, *u=0, *x=t;

	while (x) {
		v = u, u = x;
		x = text_next(u, v);
		free(u);
	}
}

int
text_insert(struct piece **t, size_t i, struct map *d, size_t o, size_t n)
{
	struct piece *u;
	int e;

	u = calloc(1, sizeof *u);
	if (!u) return ENOMEM;

	u->edit = d, u->off = o, u->len = n;
	i = text_walk(t, i);

	e = text_split(t, i, 0);
	if (e) { free(u); return e; }

	text_relink(t[0], u, t[1]);
	t[0] = u;

	return 0;
}

void
text_link(struct piece *l, struct piece *r)
{
	if (l) l->link ^= (uintptr_t)r;
	if (r) r->link ^= (uintptr_t)l;
}

void
text_merge(struct piece **t)
{
	struct piece *v;
	struct piece *x;
	size_t z;

	if (!t[1]->len) return;
	if (t[0]->edit != t[1]->edit) return;

	z = t[1]->off + t[1]->len;
	if (z != t[0]->off) return;

	t[1]->len += t[0]->len;

	x = text_next(t[0], t[1]);
	text_unlink(t[0], t[1]);
	text_unlink(x, t[0]);
	text_link(x, t[1]);

	v = t[0];  // XXX
	t[1] = x;

	free(v);
}

struct piece *
text_next(struct piece *t, struct piece *v)
{
	uintptr_t r;

	r = t->link ^ (uintptr_t)v;

	return (void *)r;
}

void
text_relink(struct piece *x, struct piece *u, struct piece *v)
{
	if (x && v) text_unlink(x, v);
	text_link(x, u);
	text_link(u, v);
}

int
text_split(struct piece **t, size_t i, size_t n)
{
	struct piece *x;
	struct piece *u;

	if (i + n == 0) return 0;

	u = calloc(1, sizeof *u);
	if (!u) return ENOMEM;

	u->len = t[0]->len - i - n;
	u->off = t[0]->off + i + n;
	u->edit = t[0]->edit;

	t[0]->len = i;

	x = text_next(t[0], t[1]);

	text_relink(x, u, t[0]);

	t[1] = t[0];
	t[0] = u;

	return 0;
}

void
text_unlink(struct piece *l, struct piece *r)
{
	l->link ^= (uintptr_t)r;
	r->link ^= (uintptr_t)l;
}

size_t
text_walk(struct piece **t, size_t i)
{
	struct piece *x;
	size_t o = 0;

	while (o + t[0]->len <= i) {
		x = text_next(t[0], t[1]);
		if (!x) break;

		o += t[0]->len;
		t[1] = t[0], t[0] = x;
	} 

	return i - o;
}
