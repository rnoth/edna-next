#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <tag.h>

#include <ln.h>
#include <util.h>

/* sorry */
#define foreach_node(N, L)	  \
	for (struct frag *_n, **_l=&(L); \
	     (_n = (N) = *_l) \
	     && (*_l = untag(_n->link[0]), true);)

static struct frag *nodes_from_lines(char *s, size_t n);

struct frag *
nodes_from_lines(char *s, size_t n)
{
	struct frag *p=0, *q=0, *r;
	size_t y=0;

	while (n) {
		y = next_line(s, n);
		s += y, n -= y;

		r = calloc(1, sizeof *r);
		if (!r) goto fail;
		if (!p) p = r;

		r->len = y;
		if (q) q->link[0] = tag0(r);
		q = r;
	}

	return p;

 fail:
	foreach_node(q, p) free(q);
	return 0x0;
}

int
ln_insert(struct frag **p, size_t x, char *s, size_t n)
{
	struct frag *q, *r;

	q = nodes_from_lines(s, n);
	if (!q) return ENOMEM;

	*p = frag_query(*p, x);

	foreach_node (r, q) {
		r->link[0]=0;
		frag_insert(*p, fozin(*p, len), r);
		*p = r;
	}

	return 0;
}

void
ln_delete(struct frag **f, size_t x, size_t n)
{
	struct frag *q, *Q=0x0;
	size_t d;

	if (!*f) return;

	*f = frag_stab(*f, &x);

	if (x) {
		d = f[0]->len - x, frag_trunc(*f, x);
		frag_offset(*f, d), *f = frag_next(*f, 1);
		n -= d;
	}

	while (f[0]->len <= n) {
		q = *f, *f = frag_next(q, 1);
		if (!*f) *f = frag_next(q, 0);
		n -= q->len;
		frag_remove(q);
		if (Q) Q->link[0] = tag0(q);
		Q = q;
		if (!*f) break;
	}

	if (n) frag_offset(*f, -n);

	foreach_node(q, Q) free(q);
}
