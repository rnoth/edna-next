#include <stdint.h>
#include <stdlib.h>

#include <frag.h>
#include <util.h>
#include <tag.h>

void
frag_delete(struct frag *fg, size_t pos)
{
	struct frag_node *match;

	match = frag_search(fg, pos);
	if (!match) return;
	fg->chld = 0;
}

int
frag_insert(struct frag *fg, struct frag_node *node)
{
	struct frag_node *match;
	int b;

	if (!fg->chld) {
		fg->chld = (uintptr_t)node;
		*node = (struct frag_node){
			.link = {0, 0},
			.off = node->off,
			.len = node->len,
			.dsp = node->len,
			.wid = 0,
		};

		return 0;
	}

	match = untag(fg->chld);

	b = node->off >= match->dsp;
	
	match->link[b] = (uintptr_t)node;

	match->dsp += node->len;
	if (b) match->wid += node->len;
	else match->off += node->len;

	node->off = 0;

	return 0;
}

void *
frag_search(struct frag *fg, size_t pos)
{
	struct frag_node *d;
	if (!fg->chld) return 0;

	d = untag(fg->chld);

	if (d->off <= pos && d->off + d->len > pos) {
		return d;
	}

	return 0;
}
