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

static struct frag *nodes_from_lines(char *buffer, size_t length);
static struct frag *link_node(struct frag *node, struct frag *list);

int
ln_insert(struct frag **f, size_t x, char *s, size_t n)
{
	struct frag *Q, *q;

	Q = nodes_from_lines(s, n);
	if (!Q) return ENOMEM;

	foreach_node (q, Q) {
		frag_insert(*f, x, q);
		x += q->len, *f = q;
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
	struct frag *q=0;
	size_t z=0;

	while (n) {
		z = next_line(s, n);

		q = calloc(1, sizeof *q);
		if (!q) goto fail;

		q->len = z;
		Q = link_node(q, Q);

		s += z, n -= z;
	}

	return Q;

 fail:
	foreach_node(q, Q) free(q);
	return 0x0;
}

void
ln_delete(struct frag **f, size_t x, size_t n)
{
	struct frag *p;
	struct frag *Q=0x0;
	struct frag *q=0x0;
	size_t z = x+n, d, b;

	b = x, p = frag_stab(*f, &b);

	if (b) {
		d = p->len - b, p->len = b;
		p = frag_next(p, 1), frag_offset(p, d);
		x += d, n -= d;
	}

	while (x < z) {
		if (p->len > n) break;

		q = p, p = frag_next(p, 1);
		x += q->len, n -= q->len;
		frag_delete(q), Q = link_node(q, Q);
	}

	if (n) {
		frag_offset(p, n);
	}

	foreach_node(q, Q) free(q);
}
