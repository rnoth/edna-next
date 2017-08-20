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

static struct ext_node *tree_detatch(struct ext_walker *walker,
                                     size_t offset, size_t extent);
static void tree_marshal(struct ext_walker *walker);
static ptrdiff_t tree_prune(struct ext_walker *walker, size_t off, int b);

static void walker_adjust(struct ext_walker *walker);
static void walker_begin(struct ext_walker *walker, struct ext *ext);
static void walker_locate(struct ext_walker *walker, size_t offset, size_t extent);
static void walker_rise(struct ext_walker *walker);
static void walker_surface(struct ext_walker *walker);
static void walker_visit(struct ext_walker *walker, int b);

void
ext_append(struct ext *ext, struct ext_node *new_node)
{
	ext_insert(ext, ext->len, new_node);
}

void *
ext_continue(struct ext_walker *walker)
{
	struct ext_node *node;
	int b;

	if (!walker->tag) {
		return 0;
	}

	if (is_root(walker->prev)) {
		walker_locate(walker, 0, 0);
		node = untag(walker->tag);
		if (is_root(walker->prev)) {
			*walker = (struct ext_walker){0};
		}
		return node;
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
	walker_locate(walker, 0, 0);

	return untag(walker->tag);
}

void
ext_extend(struct ext *ext, size_t offset, ptrdiff_t adjust)
{
	struct ext_walker walker[1];
	struct ext_node *node;

	walker_begin(walker, ext);
	walker_locate(walker, offset, 0);

	node = untag(walker->tag);
	if (offset - walker->off >= node->ext) return;

	node->ext += adjust;

	walker->adj = adjust;
	walker_surface(walker);
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
ext_insert(struct ext *ext, size_t offset, struct ext_node *new)
{
	struct ext_walker walker[1];

	if (!ext->root) {
		new->chld[0]=0, new->chld[1]=0;
		new->off = new->ext;

		ext->off = offset;
		ext->len = new->ext;
		ext->root = tag_leaf(new);
		return;
	}

	walker_begin(walker, ext);
	walker_locate(walker, offset, 0);

	new->off = offset;

	node_insert(walker, new);
	walker_surface(walker);
}

void
ext_iterate(struct ext_walker *walker, struct ext *ext)
{
	walker_begin(walker, ext);
}

void
ext_offset(struct ext *ext, size_t offset, ptrdiff_t adjust)
{
	struct ext_walker walker[1];
	struct ext_node *node;

	walker_begin(walker, ext);
	walker_locate(walker, offset, 0);

	node = untag(walker->tag);
	if (offset - walker->off >= node->ext) return;

	walker->adj = adjust;
	walker_surface(walker);
}

void *
ext_remove(struct ext *ext, size_t offset, size_t extent)
{
	struct ext_walker walker[1];
	struct ext_node *result;

	if (!ext->root) return 0x0;

	walker_begin(walker, ext);
	walker_locate(walker, offset, extent);
	result = untag(walker->tag);

	if (is_leaf(walker->tag)) {
		if (offset - walker->off > result->ext) {
			walker_rise(walker);
			return 0x0;
		}

		if (is_root(walker->prev)) {
			ext->root = 0;
			ext->off = ext->len = 0;
			return result;
		}

		node_detatch(walker);
		walker_surface(walker);

		return result;
	}

	tree_detatch(walker, offset, extent);

	__builtin_trap();
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

void *
ext_walk(struct ext_walker *walker, size_t offset)
{
	walker_locate(walker, offset, 0);
	return untag(walker->tag);
}

void
node_detatch(struct ext_walker *walker)
{
	struct ext_node *great;
	struct ext_node *prev;
	struct ext_node *del;
	int b;

	del = untag(walker->tag);

	walker->adj = -del->ext;

	prev = untag(walker->prev);
	b = is_back(prev->chld[1]);
	walker->tag = prev->chld[!b];
	walker->prev = flip_tag(prev->chld[b]);

	prev->chld[0] = 0, prev->chld[1] = 0;

	if (is_root(walker->prev)) {
		untag_ext(walker->prev)->len -= del->ext;
		return;
	}

	prev->off = del->off;
	prev->chld[0] = del->chld[0];
	prev->chld[1] = del->chld[1];

	great = untag(walker->prev);
	b = is_back(great->chld[1]);
	great->off -= b ? 0 : del->ext;
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
	new->off = b ? new->off - walker->off : new->ext;

	new->chld[ b] = flip_tag(walker->prev);
	new->chld[!b] = walker->tag;

	walker->prev = tag_node(new);
	walker->tag = tag_leaf(new);
	walker->adj = new->ext;
}

struct ext_node *
tree_detatch(struct ext_walker *walker, size_t offset, size_t extent)
{
	struct ext_node *node;
	struct ext_node *result;
	uintptr_t ancestor;
	uintptr_t new;
	size_t rel_off;
	int b;

	ancestor = walker->tag;

	offset += tree_prune(walker, offset, 0);
	offset += tree_prune(walker, offset+extent, 1);

	rel_off = offset - walker->off;
	walker_locate(walker, rel_off, extent);

	tree_marshal(walker);
	result = untag(walker->tag);

	if (is_root(walker->prev)) {
		walker->tag = 0;
		return result;
	}

	node = untag(walker->prev);
	b = is_back(node->chld[1]);
	new = node->chld[b];

	walker->prev = flip_tag(node->chld[b]);
	walker->tag = new;
	return result;
}

void
tree_marshal(struct ext_walker *walker)
{
	//struct ext_node *list;
	struct ext_node *node;
	uintptr_t ancestor;
	int b;

	ancestor = walker->prev;

 descend:
	if (is_leaf(walker->tag)) {
		node = untag(walker->tag);
		if (!node->chld[1]) goto link;
		else goto rise;
	}

	node = untag(walker->tag);
	b = !node->chld[0];
	if (b && !node->chld[1]) goto link;

	walker_visit(walker, b);
	goto descend;

 link:
	__builtin_trap();
	/* if (walker->prev == ancestor) goto done; */
	/* goto rise; */

 rise:
	node = untag(walker->prev);
	b = is_back(node->chld[1]);

	walker->tag = walker->prev;
	walker->prev = flip_tag(node->chld[b]);
	node->chld[b] = 0;

	goto descend;

 /* done: */
 /* 	__builtin_trap(); */
}

ptrdiff_t
tree_prune(struct ext_walker *walker, size_t offset, int b)
{
	struct ext_node *node;
	uintptr_t ancestor;
	size_t off;
	size_t lef;
	size_t rit;

	ancestor = walker->tag;

	off = offset - walker->off;
	walker_locate(walker, off, 0);

	node = untag(walker->tag);
	lef = offset - walker->off;
	rit = node->ext - lef;

	walker->adj += b ? -lef : -rit;

	node->ext += walker->adj;
	walker_adjust(walker);

	while (walker->tag != ancestor) walker_rise(walker);

	return b ? -lef : -rit;
}

void
walker_adjust(struct ext_walker *walker)
{
	struct ext_node *node;
	struct ext *ext;
	int b;

	if (is_root(walker->prev)) {
		ext = untag(walker->prev);
		ext->len += walker->adj;
		return;
	}

	node = untag(walker->prev);
	b = is_back(node->chld[1]);
	node->off += b ? 0 : walker->adj;
}

void
walker_begin(struct ext_walker *walker, struct ext *ext)
{
	walker->prev = tag_root(ext);
	walker->tag = ext->root;
	walker->off = ext->off;
	walker->len = ext->len;
	walker->adj = 0;
}

void
walker_locate(struct ext_walker *walker, size_t offset, size_t extent)
{
	struct ext_node *node;
	int b;

	while (is_node(walker->tag)) {
		node = untag(walker->tag);
		b = offset >= walker->off + node->off;

		walker->len = b ? walker->len - node->off : node->off;
		if (walker->len - walker->off < extent) break;

		walker_visit(walker, b);
	}
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

	walker_adjust(walker);
}

void
walker_surface(struct ext_walker *walker)
{
	while (!is_root(walker->prev)) walker_rise(walker);

	walker_rise(walker);
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
