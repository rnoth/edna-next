#include <stdint.h>
#include <stdlib.h>

#include <frag.h>
#include <tag.h>

void
frag_insert(struct frag *fg, struct frag_node *node)
{
	if (!fg->chld) {
		fg->chld = (uintptr_t)node;
		*node = (struct frag_node){
			.link = {0, 0},
			.off = node->off,
			.len = node->len,
			.dsp = node->len,
			.wid = node->len,
		};
	}
}

void *
frag_search(struct frag *fg, size_t pos)
{
	return 0;
}
