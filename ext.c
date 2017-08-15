#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <tag.h>
#include <util.h>

#include <ext.h>

struct walker {
	uintptr_t prev;
	uintptr_t tag;
	size_t off;
};

static void node_insert(struct walker *walker, struct ext_node *new_node);
static void node_shift(struct walker *walker, size_t offset);

static void walker_begin(struct walker *walker, struct ext *ext);
static void walker_rise(struct walker *walker);
static void walker_surface(struct walker *walker);
static void walker_walk(struct walker *walker, size_t p);

void
node_insert(struct walker *walker, struct ext_node *new_node)
{
	struct ext_node *node;
	struct ext *ext;
	size_t new_end;
	size_t new_crit;
	size_t crit;
	size_t end;

	new_end = new_node->off + new_node->ext;
	new_crit = ufls(new_end ^ new_node->off);

	while (walker_rise(walker), is_node(walker->prev)) {

		node = untag(walker->prev);
		end = walker->off + node->ext;
		crit = ufls(new_end ^ end);

		if (new_crit < crit) goto done;
	}

	ext = untag(walker->prev);
	ext->root = tag_node(new_node);

 done:
	new_node->chld[0] = walker->tag;
	new_node->chld[1] = tag_leaf(new_node);

	walker->tag = tag_node(new_node);
}

void
node_shift(struct walker *walker, size_t offset)
{
	struct ext_node *node;
	int b;

	while (is_node(walker->prev)) {
		node = untag(walker->tag);
		b = is_back(node->chld[1]);
		node->off += b ? 0 : offset;

		walker_rise(walker);
	}
}

void
walker_begin(struct walker *walker, struct ext *ext)
{
	walker->prev = tag_root(ext);
	walker->tag = ext->root;
	walker->off = 0;
}

void
walker_rise(struct walker *walker)
{
	struct ext_node *node;
	int b;

	if (is_root(walker->prev)) return;

	node = untag(walker->prev);
	b = is_back(node->chld[1]);

	walker->prev = flip_tag(node->chld[b]);
	node->chld[b] = walker->tag;

	walker->tag = tag_node(node);
	walker->off -= node->chld[b];
}

void
walker_surface(struct walker *walker)
{
	while (!is_root(walker->prev)) {
		walker_rise(walker);
	}
}

void
walker_visit(struct walker *walker, int b)
{
	struct ext_node *node;
	uintptr_t next;

	if (is_leaf(walker->tag)) return;

	node = untag(walker->tag);
	walker->off += b ? node->off : 0;

	next = node->chld[b];
	node->chld[b] = flip_tag(walker->prev);

	walker->prev = walker->tag;
	walker->tag = next;
}

void
walker_walk(struct walker *walker, size_t p)
{
	struct ext_node *node;
	int b;

	while (is_node(walker->tag)) {
		node = untag(walker->tag);
		b = p >= walker->off + node->off;
		walker_visit(walker, b);
	}
}

void
ext_append(struct ext *ext, struct ext_node *new_node)
{
	struct walker walker[1];
	struct ext_node *node;

	if (!ext->root) {
		new_node->chld[0] = tag_leaf(ext->zero);
		new_node->chld[1] = tag_leaf(new_node);
		new_node->off = 0;
		ext->root = tag_node(new_node);
		return;
	}

	walker_begin(walker, ext);
	walker_walk(walker, ~0);

	node = untag(walker->tag);
	new_node->off = walker->off + node->ext;

	node_insert(walker, new_node);

	walker_surface(walker);

	return;
}

void
ext_insert(struct ext *ext, struct ext_node *new_node, size_t offset)
{
	struct walker walker[1];

	if (!ext->root) {
		new_node->off = offset;
		new_node->chld[0] = tag_leaf(ext->zero);
		new_node->chld[1] = tag_leaf(new_node);
		ext->root = tag_node(new_node);
		return;
	}

	walker_begin(walker, ext);
	walker_walk(walker, offset);

	new_node->off = offset - walker->off;

	node_insert(walker, new_node);
	node_shift(walker, new_node->off);
}

struct ext_node *
ext_stab(struct ext *ext, size_t point)
{
	struct ext_node *node;
	uintptr_t tag;
	size_t off;
	int b;

	if (!ext->root) return 0x0;

	tag = ext->root;
	off = 0;
	while (is_node(tag)) {
		node = untag(tag);
		b = point >= off + node->off;
		off += b ? node->off : 0;
		tag = node->chld[b];
	}

	node = untag(tag);
	node = (off + node->ext > point) ? node : 0x0;

	return node;
}
