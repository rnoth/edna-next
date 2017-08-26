#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <ext.h>
#include <tag.h>

#include <ln.h>

// I'm so sorry
#define foreach_node(NODE, LIST) \
	for (struct ext_node *_node, **_listp; \
	     _listp = &(LIST), \
	     (_node = (NODE) = *_listp) \
	     && (*_listp = untag(_node->chld[0]), true);)

static size_t next_line(char *buffer, size_t length);
static struct ext_node *nodes_from_lines(char *buffer, size_t length);
static struct ext_node *link_node(struct ext_node *node, struct ext_node *list);

int
ln_insert(struct ext *lines, size_t start, char *buffer, size_t length)
{
	struct ext_node *list, *node;

	list = nodes_from_lines(buffer, length);
	if (!list) return ENOMEM;

	foreach_node (node, list) {
		ext_insert(lines, start, node);
	}

	return 0;
}

struct ext_node *
link_node(struct ext_node *node, struct ext_node *list)
{
	node->chld[0] = tag0(list);
	return node;
}

size_t
next_line(char *buffer, size_t length)
{
	char *nl = memchr(buffer, '\n', length);
	if (!nl) return length;
	return nl - buffer + 1;
}

struct ext_node *
nodes_from_lines(char *buffer, size_t length)
{
	struct ext_node *list=0x0;
	struct ext_node *node=0;
	size_t extent=0;
	size_t offset=0;

	while ((offset += extent) < length) {
		extent = next_line(buffer + offset, length - offset);

		node = calloc(1, sizeof *node);
		if (!node) goto fail;

		node->ext = extent;
		list = link_node(node, list);
	}

	return list;

 fail:
	foreach_node(node, list) free(node);
	return 0x0;
}

void
ln_delete(struct ext *lines, size_t offset, size_t extent)
{
	struct ext_node *dead;
	struct ext_node *list=0x0;
	struct ext_node *node=0x0;
	size_t endoff = offset + extent;
	size_t diff;
	size_t start;

	start = ext_tell(lines, offset);
	if (start < offset) {
		node = ext_stab(lines, offset);
		ext_adjust(lines, offset, offset - start + node->ext);

		diff = offset - start + node->ext;
		offset += diff;
	}

	while (offset < endoff) {
		node = ext_stab(lines, offset);
		if (node->ext > extent) break;

		dead = ext_remove(lines, offset);
		offset += dead->ext;
		list = link_node(dead, list);
	}

	ext_adjust(lines, offset, offset - endoff);

	foreach_node(dead, list) free(dead);

	return;
}
