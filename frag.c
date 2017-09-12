#include <stdint.h>
#include <stdlib.h>

#include <frag.h>
#include <util.h>
#include <tag.h>

static struct frag_node *frag_find(struct frag *, size_t pos);

void
set_chld(struct frag_node *node, uintptr_t chld, int b, uintptr_t prnt)
{
	node->link[b] = chld ^ prnt;
}

void
set_prnt(struct frag_node *node, uintptr_t new, uintptr_t old)
{
	node->link[0] ^= new ^ old;
	node->link[1] ^= new ^ old;
}

uintptr_t
get_chld(struct frag_node *node, uintptr_t prnt, int b)
{
	return node->link[b] ^ prnt;
}

uintptr_t
get_prnt(struct frag_node *node, uintptr_t chld, int b)
{
	return node->link[b] ^ chld;
}

void
frag_delete(struct frag *fg, size_t pos)
{
	struct frag_node *match;

	match = frag_stab(fg, pos);
	if (!match) return;
	fg->chld = 0;
}

struct frag_node *
frag_find(struct frag *fg, size_t pos)
{
	struct frag_node *node;
	int b;

	node = untag(fg->chld);

	while (!in_range(node->off, node->len, pos - fg->dsp)) {
		b = pos >= node->dsp;
		if (!get_chld(node, b, fg->prnt)) return node;

		fg->dsp += b ? node->dsp : 0;
		lshift(&fg->prnt, &fg->chld, get_chld(node, b, fg->prnt));
		node = untag(fg->chld);
	}

	return node;
}

void
frag_flush(struct frag *fg)
{
	__builtin_trap();
}

int
frag_insert(struct frag *fg, struct frag_node *node)
{
	struct frag_node *match;
	int b;

	if (!fg->chld) {
		fg->prnt = 0;
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

	match = frag_find(fg, node->off);

	b = node->off >= match->dsp;

	set_chld(match, b, (uintptr_t)node, fg->prnt);

	match->dsp += node->len;
	if (b) match->wid += node->len;
	else match->off += node->len;

	node->off = 0;

	lshift(&fg->prnt, &fg->chld, get_chld(match, b, fg->prnt));

	return 0;
}

void *
frag_stab(struct frag *fg, size_t pos)
{
	struct frag_node *match;

	if (!fg->chld) return 0;

	match = frag_find(fg, pos);
	if (!in_range(match->off, match->len, pos - fg->dsp)) {
		return 0;
	}

	return match;
}
