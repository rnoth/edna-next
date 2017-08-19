#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <tag.h>
#include <util.h>

#include <ext.h>

#define untag_ext(t) ((struct ext *)untag(t))

static void node_insert(struct ext_walker *walker, struct ext_node *new_node);
static void node_detatch(struct ext_walker *walker);
static void node_shift(struct ext_walker *walker, size_t offset);

static void walker_begin(struct ext_walker *walker, struct ext *ext);
static void walker_rise(struct ext_walker *walker);
static void walker_surface(struct ext_walker *walker);
static void walker_visit(struct ext_walker *walker, int b);
static void walker_walk(struct ext_walker *walker, size_t p);

void
ext_append(struct ext *ext, struct ext_node *new_node)
{
	ext_insert(ext, new_node, ext->len);
}

void *
ext_continue(struct ext_walker *walker)
{
	struct ext_node *node;
	int b;

	if (!walker->prev) {
		return 0;
	}

	if (is_root(walker->prev)) {
		*walker = (struct ext_walker){0};
		return 0;
	}

	while (!is_root(walker->prev)) {
		node = untag(walker->prev);
		b = is_back(node->chld[1]);
		walker_rise(walker);
		if (!b) goto cont;
	}

	*walker = (struct ext_walker){0};
	return 0;

 cont:
	walker_visit(walker, 1);
	walker_walk(walker, 0);

	return untag(walker->tag);
}

void
ext_free(struct ext *ext)
{
	struct ext_walker walker[1];
	struct ext_node *node;
	int b;

	if (!ext->root) return;

	walker_begin(walker, ext);

 descend:
	if (is_leaf(walker->tag)) {
		node = untag(walker->tag);
		if (!node->chld[1]) goto free;
		else goto rise;
	}

	node = untag(walker->tag);
	b = !node->chld[0];
	if (b && !node->chld[1]) goto free;

	walker_visit(walker, b);
	goto descend;

 free:
	free(node);
	if (is_root(walker->prev)) return;
	goto rise;

 rise:
	node = untag(walker->prev);
	b = is_back(node->chld[1]);

	walker->tag = walker->prev;
	walker->prev = flip_tag(node->chld[b]);
	node->chld[b] = 0;

	goto descend;
}

void
ext_insert(struct ext *ext, struct ext_node *new, size_t offset)
{
	struct ext_walker walker[1];{}

	if (!ext->root) {
		new->chld[0]=0, new->chld[1]=0;
		new->off = new->ext;
		ext->off = offset;
		ext->len = new->ext;
		ext->root = tag_leaf(new);
		return;
	}

	walker_begin(walker, ext);
	walker_walk(walker, offset);

	new->off = offset;

	node_insert(walker, new);
	node_shift(walker, new->ext);

	walker_surface(walker);
}

void *
ext_iterate(struct ext_walker *walker, struct ext *ext)
{
	walker_begin(walker, ext);
	if (ext->root) walker_walk(walker, 0);
	return untag(walker->tag);
}

void *
ext_remove(struct ext *ext, size_t offset)
{
	struct ext_walker walker[1];
	void *result;

	walker_begin(walker, ext);
	walker_walk(walker, offset);
	result = untag(walker->tag);

	node_detatch(walker);

	return result;
}

void *
ext_stab(struct ext *ext, size_t point)
{
	struct ext_node *node;
	uintptr_t tag;
	size_t off;
	int b;

	if (!ext->root) return 0x0;

	tag = ext->root;
	off = ext->off;
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

void
node_detatch(struct ext_walker *walker)
{
	struct ext_node *prev;
	struct ext_node *del;
	uintptr_t sib;
	int b;

	del = untag(walker->tag);

	prev = untag(walker->prev);
	b = is_back(prev->chld[1]);
	sib = prev->chld[!b];

	walker_rise(walker);
	prev->chld[0] = 0, prev->chld[1] = 0;
	if (!is_root(walker->prev)) {
		walker->tag = sib;
		return;
	}

	prev->off = del->off;
	prev->chld[0] = del->chld[0];
	prev->chld[1] = del->chld[1];
}

void
node_insert(struct ext_walker *walker, struct ext_node *new)
{
	struct ext_node *leaf;
	struct ext_node *node;
	size_t ext;
	size_t off;
	int b;


	leaf = untag(walker->tag);
	b = new->off >= walker->off + leaf->ext;
	off = new->off - walker->off;
	off += b ? 0 : new->ext;

	while (is_node(walker->prev)) {
		node = untag(walker->prev);

		if (off < node->off) goto done;

		b = is_back(node->chld[1]);
		off = new->off - walker->off;
		off += b ? node->off : 0;
		walker_rise(walker);

	}

 done:
	node = untag(walker->tag);

	ext = is_node(walker->tag) ? node->off : node->ext;

	b = new->off >= walker->off + ext;
	new->off = b ? new->off : new->ext;

	new->chld[ b] = tag_leaf(new);
	new->chld[!b] = walker->tag;

	walker->tag = tag_node(new);
}

void
node_shift(struct ext_walker *walker, size_t offset)
{
	struct ext_node *node;
	struct ext *ext;
	int b;

	while (is_node(walker->prev)) {
		node = untag(walker->tag);
		b = is_back(node->chld[1]);
		node->off += b ? 0 : offset;

		walker_rise(walker);
	}

	ext = untag(walker->prev);
	ext->len += offset;
}

void
walker_begin(struct ext_walker *walker, struct ext *ext)
{
	walker->prev = tag_root(ext);
	walker->tag = ext->root;
	walker->off = ext->off;
}

void
walker_rise(struct ext_walker *walker)
{
	struct ext_node *node;
	struct ext *ext;
	int b;

	if (is_root(walker->prev)) {
		ext = untag(walker->prev);
		ext->root = walker->tag;
		return;
	}

	node = untag(walker->prev);
	b = is_back(node->chld[1]);

	walker->prev = flip_tag(node->chld[b]);
	node->chld[b] = walker->tag;

	walker->tag = tag_node(node);
	walker->off -= b ? node->off : 0;
}

void
walker_surface(struct ext_walker *walker)
{
	do walker_rise(walker);
	while (!is_root(walker->prev));
}

void
walker_visit(struct ext_walker *walker, int b)
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
walker_walk(struct ext_walker *walker, size_t p)
{
	struct ext_node *node;
	int b;

	while (is_node(walker->tag)) {
		node = untag(walker->tag);
		b = p >= walker->off + node->off;
		walker_visit(walker, b);
	}
}

