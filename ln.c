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
static struct frag *link_node(struct frag *q, struct frag *Q);

int
ln_insert(struct frag **f, size_t x, char *s, size_t n)
{
	struct frag *Q, *q;

	Q = nodes_from_lines(s, n);
	if (!Q) return ENOMEM;

	*f = frag_query(*f, x);

	foreach_node (q, Q) {
		frag_insert(*f, *f ? f[0]->len : 0, q);
		*f = q;
	}

	return 0;
}

struct frag *
link_node(struct frag *q, struct frag *Q)
{
	q->link[0] = tag0(Q);
	return q;
}

struct frag *
nodes_from_lines(char *s, size_t n)
{
	struct frag *Q=0x0;
	struct frag *q=0x0;
	size_t y=0;

	while (n) {
		y = next_line(s, n);
		s += y, n -= y;

		q = calloc(1, sizeof *q);
		if (!q) goto fail;

		q->len = y;
		Q = link_node(q, Q);
	}

	return Q;

 fail:
	foreach_node(q, Q) free(q);
	return 0x0;
}

void
ln_delete(struct frag **f, size_t x, size_t n)
{
	struct frag *Q=0x0;
	struct frag *q=0x0;
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
		frag_remove(q), Q = link_node(q, Q);
		if (!*f) break;
	}

	if (n) frag_offset(*f, -n);

	foreach_node(q, Q) free(q);
}
