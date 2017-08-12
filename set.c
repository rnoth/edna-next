#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <util.h>
#include <set.h>

#define tag_back(u) ((u) | 2)
#define tag_leaf(n) ((uintptr_t)n | 1)
#define tag_node(n) ((uintptr_t)n)
#define tag_set(s) ((uintptr_t)s | 1)

#define node_from_tag(u) ((struct set_node *)(u & ~3))
#define set_from_tag(u) ((struct set *)(u & ~3))
#define tag_from_back(u) (u ^ 2)

#define is_back(u) !!(u & 2)
#define is_leaf(u) (u & 1)
#define is_node(u) !(u & 1)
#define is_set(u) (u & 1)

#define obj(n) 

typedef int8_t bit;

struct walker {
	uintptr_t cur;
	uintptr_t prev;
	size_t dep:LONG_BIT-3;
	size_t bit:1;
};

static bit bit_index_bytes(uint8_t *bytes, size_t len, size_t crit);
static size_t byte_diff(void *lef, void *rit, size_t len);
static void node_attach(struct walker *walk, struct set_node *el_node,
                        uint8_t *key, size_t len);
static uintptr_t node_traverse(uintptr_t root, uint8_t *, size_t len);
static void walker_begin(struct walker *, struct set *);
static void walker_finish(struct walker *);
//static void walker_next(struct walker *);
static void walker_rise(struct walker *);
static void walker_visit(struct walker *, bit);
static void walker_walk(struct walker *, void *, size_t);

static inline void lrotate(ulong *lef, ulong *mid, ulong *rit);

void
lrotate(ulong *lef, ulong *mid, ulong *rit)
{
	ulong tmp = *lef;
	*lef = *mid;
	*mid = *rit;
	*rit = tmp;
}

bit
bit_index_bytes(uint8_t *key, size_t len, size_t crit)
{
	size_t byt = crit / 8;
	size_t bit = crit % 8;

	if (byt >= len) return 2;

	return !!(key[byt] & 128 >> bit);
}

size_t
byte_diff(void *_lef, void *_rit, size_t len)
{
	uint8_t *lef=_lef, *rit=_rit;
	size_t off;

	off=len;
	while (--off) {
		if (*lef != *rit) goto diff;
		++lef, ++rit;
	}

	return -1;
 diff:
	off = len - off - 1;
	return off * 8 + 8 - ufls(*lef ^ *rit);
}

void
node_attach(struct walker *walk, struct set_node *el_node,
            uint8_t *key, size_t len)
{
	uintptr_t *dest_tag_ptr;
	struct set_node *cur_node;
	bit b;

	b = bit_index_bytes(el_node->key, len, el_node->crit);
	el_node->chld[b] = tag_leaf(el_node);
	
	while (!is_set(walk->prev)) {
		walker_rise(walk);

		cur_node = node_from_tag(walk->cur);
		dest_tag_ptr = cur_node->chld + walk->bit;
		if (cur_node->crit < el_node->crit) goto attach;
	}

	dest_tag_ptr = &set_from_tag(walk->prev)->root;

 attach:
	el_node->chld[!b] = *dest_tag_ptr;
	*dest_tag_ptr = tag_node(el_node);

	++walk->dep;
}

uintptr_t
node_traverse(uintptr_t tag, uint8_t *key, size_t len)
{
	struct set_node *node;
	int b;

	while (is_node(tag)) {
		node = node_from_tag(tag);
		b = bit_index_bytes(key, len, node->crit);
		if (b == 2) return 0;
		tag = node->chld[b];
	}

	return tag;
}

void
walker_begin(struct walker *wal, struct set *set)
{
	wal->prev = tag_set(set);
	wal->cur = set->root;
	wal->bit = 0;
	wal->dep = 0;
}

void
walker_finish(struct walker *wal)
{
	while (!is_set(wal->prev)) {
		walker_rise(wal);
	}
}

void
walker_rise(struct walker *wal)
{
	uintptr_t *chld;
	bit b;

	if (is_set(wal->prev)) return;
	
	chld = node_from_tag(wal->prev)->chld;
	b = is_back(chld[1]);
	chld[b] = tag_from_back(chld[b]);

	lrotate(chld+b, &wal->cur, &wal->prev);

	wal->bit = b;
	--wal->dep;
}

void
walker_visit(struct walker *wal, bit b)
{
	uintptr_t *chld;

	chld = node_from_tag(wal->cur)->chld;
	lrotate(chld+b, &wal->prev, &wal->cur);
	chld[b] = tag_back(chld[b]);

	wal->bit = b;
	if (is_node(wal->cur)) ++wal->dep;
}

void
walker_next(struct walker *walk) // probably doesn't work
{
	struct set_node *node;

	if (is_leaf(walk->cur)) {
		walker_rise(walk);
		return;
	}

	node = node_from_tag(walk->cur);

	if (!is_back(node->chld[0])) {
		walker_visit(walk, 0);
		return;
	}

	if (!is_back(node->chld[1])) {
		walker_visit(walk, 1);
		return;
	}

	walker_rise(walk);
}

void
walker_walk(struct walker *wal, void *key, size_t len)
{
	struct set_node *cur_node;
	bit b;

	while (is_node(wal->cur)) {
		cur_node = node_from_tag(wal->cur);
		b = bit_index_bytes(key, len, cur_node->crit);
		walker_visit(wal, b % 2);
	}
}

void
set_add(struct set *set, struct set_node *new, size_t len)
{
	struct set_node *node = (void *)new;
	set_add_key(set, new, node->key, len);
}

void
set_add_key(struct set *set, struct set_node *new,
            void *key, size_t len)
{
	struct walker walk[1];
	struct set_node *new_node;
	struct set_node *sib_node;
	size_t diff;

	if (!set) return;
	if (!new) return;

	new_node = (void *)new;

	if (!set->root) {
		set->root = tag_leaf(new_node);
		return;
	}

	walker_begin(walk, set);
	walker_walk(walk, key, len);

	sib_node = node_from_tag(walk->cur);
	diff = byte_diff(sib_node->key, key, len);
	if (!~diff) goto done;
	new_node->crit = diff;

	node_attach(walk, new_node, key, len);

	if (walk->dep > set->height) {
		set->height = walk->dep;
	}
 done:
	walker_finish(walk);
}

void *
set_rm(struct set *set, void *key, size_t len)
{
	return 0;
}

bool
set_has(struct set *set, void *key, size_t len)
{
	struct set_node *node;
	uintptr_t tag;
	
	if (!set) return false;
	if (!key) return false;

	tag = node_traverse(set->root, key, len);
	node = node_from_tag(tag);
	return byte_diff(node->key, key, len) == -1UL;
}

void *
set_query(struct set *set, void *key, size_t len)
{
	return node_from_tag(node_traverse(set->root, key, len));
}
