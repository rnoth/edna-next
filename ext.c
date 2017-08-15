#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <util.h>

#include <ext.h>

struct walker {
	uintptr_t prev;
	uintptr_t tag;
	size_t off;
};

static bool is_node(uintptr_t tag);
static struct ext_node *node_from_tag(uintptr_t tag);
static uintptr_t tag_node(struct ext_node *node);
static uintptr_t tag_back(uintptr_t tag);

static ulong sig(ulong a) {return a & -a;} 

static bool is_back(uintptr_t tag) { return !!(tag & 2); }
static bool is_leaf(uintptr_t tag) { return tag & 1; }
static bool is_node(uintptr_t tag) { return !(tag & 1); }
static bool is_ext(uintptr_t tag) { return tag & 1; }

static struct ext *ext_from_tag(uintptr_t tag) { return (void *)(tag & ~3); }
static struct ext_node *node_from_tag(uintptr_t tag) {return (void *)(tag & ~3); }
static uintptr_t tag_from_back(uintptr_t back) { return back & ~2; }

static uintptr_t tag_back(uintptr_t tag) { return tag | 2; }
static uintptr_t tag_leaf(struct ext_node *leaf) { return (uintptr_t)leaf | 1; }
static uintptr_t tag_node(struct ext_node *node) { return (uintptr_t)node; }
static uintptr_t tag_ext(struct ext *ext) { return (uintptr_t)ext | 1; }

static void node_insert(struct walker *walker, struct ext_node *new_node);

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
	size_t end;
	size_t crit;

	node = node_from_tag(walker->tag);
	end = walker->off + node->ext;
	new_node->off = end;

	new_end = end + new_node->ext;
	new_crit = sig(new_end ^ end);

	while (walker_rise(walker), is_node(walker->prev)) {

		node = node_from_tag(walker->prev);
		end = walker->off + node->ext;
		crit = sig(new_end ^ end);

		if (new_crit < crit) {
			new_node->chld[0] = walker->tag;
			new_node->chld[1] = tag_leaf(new_node);
			walker->tag = tag_node(new_node);
			return;
		}
	}

	ext = ext_from_tag(walker->prev);
	new_node->chld[0] = ext->root;
	new_node->chld[1] = tag_leaf(new_node);
	ext->root = tag_node(new_node);
}

void
walker_begin(struct walker *walker, struct ext *ext)
{
	walker->prev = tag_ext(ext);
	walker->tag = ext->root;
	walker->off = ext->off;
}

void
walker_rise(struct walker *walker)
{
	struct ext_node *node;
	int b;

	if (is_ext(walker->prev)) return;

	node = node_from_tag(walker->prev);
	b = is_back(node->chld[1]);

	walker->prev = tag_from_back(node->chld[b]);
	node->chld[b] = walker->tag;

	walker->tag = tag_node(node);
	walker->off -= node->chld[b];
}

void
walker_surface(struct walker *walker)
{
	while (!is_ext(walker->prev)) {
		walker_rise(walker);
	}
}

void
walker_visit(struct walker *walker, int b)
{
	struct ext_node *node;
	uintptr_t next;

	if (is_leaf(walker->tag)) return;

	node = node_from_tag(walker->tag);
	walker->off += b ? node->off : 0;

	next = node->chld[b];
	node->chld[b] = tag_back(walker->prev);

	walker->prev = walker->tag;
	walker->tag = next;
}

void
walker_walk(struct walker *walker, size_t p)
{
	struct ext_node *node;
	int b;

	while (is_node(walker->tag)) {
		node = node_from_tag(walker->tag);
		b = p >= walker->off + node->off;
		walker_visit(walker, b);
	}
}

void
ext_append(struct ext *ext, struct ext_node *new_node)
{
	struct walker walker[1];

	new_node->off = ext->off;

	if (!ext->root) {
		ext->root = tag_leaf(new_node);
		return;
	}

	walker_begin(walker, ext);
	walker_walk(walker, ~0);

	node_insert(walker, new_node);

	walker_surface(walker);
	return;
}

void
ext_insert(struct ext *ext, struct ext_node *new_node, size_t offset)
{
	struct walker walker[1];
	new_node->off = 0;

	if (!ext->root) {
		ext->root = tag_leaf(new_node);
		ext->off = offset;
		return;
	}

	walker_begin(walker, ext);
	walker_walk(walker, offset + new_node->ext);

	__builtin_trap();
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
		node = node_from_tag(tag);
		b = point >= off + node->off;
		off += b ? node->off : 0;
		tag = node->chld[b];
	}

	node = node_from_tag(tag);
	node = (off + node->ext > point) ? node : 0x0;

	return node;
}
