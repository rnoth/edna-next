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

#define node_from_tag(u) ((struct node *)(u & ~1))
#define set_from_tag(u) ((struct set *)(u & ~1))
#define tag_from_back(u) (u ^ 2)

#define is_back(u) !!(u & 2)
#define is_leaf(u) (u & 1)
#define is_node(u) !(u & 1)
#define is_set(u) (u & 1)

#define obj(n) 

typedef int8_t bit;

struct node {
	uintptr_t chld[2];
	size_t crit;
	uint8_t obj[];
};

struct walker {
	uintptr_t cur;
	uintptr_t prev;
	size_t diff;
	uint8_t bit:1;
};

static bit bit_index_bytes(uint8_t *, size_t, size_t);
static size_t byte_diff(void *lef, void *rit, size_t len);
static void node_attach(struct walker *walk, struct node *el_node,
                        struct node *sib_node, uint8_t *key, size_t len);
static struct node *node_antilocate(struct walker *, uint8_t *, size_t);
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

	for (off=0; off<len; ++off) {
		if (*lef != *rit) goto diff;
		++lef, ++rit;
	}

	return -1;
 diff:
	return off * 8 + 8 - ufls(*lef ^ *rit);
}

struct node *
node_antilocate(struct walker *walk, uint8_t *key, size_t len)
{
	struct node *match;

	walker_walk(walk, key, len);
	match = node_from_tag(walk->cur);

	walk->diff = byte_diff(match->obj, key, len);
	return walk->diff != -1UL ? match : 0;
}

void
node_attach(struct walker *walk, struct node *el_node,
            struct node *sib_node, uint8_t *key, size_t len)
{
	uintptr_t *dest_tag_ptr;
	struct node *cur_node;
	bit b;

	el_node->crit = walk->diff;
	b = bit_index_bytes(el_node->obj, len, el_node->crit);
	el_node->chld[b] = tag_leaf(el_node);
	
	while (!is_set(walk->prev)) {
		walker_rise(walk);

		cur_node = node_from_tag(walk->cur);
		dest_tag_ptr = cur_node->chld + walk->bit;
		if (cur_node->crit < el_node->crit) goto attach;
	}

	dest_tag_ptr = &set_from_tag(walk->prev)->root;

 attach:
	b = !el_node->chld[1];
	el_node->chld[b] = *dest_tag_ptr;
	*dest_tag_ptr = tag_node(el_node);
}

void
walker_begin(struct walker *wal, struct set *set)
{
	wal->prev = tag_set(set);
	wal->cur = set->root;
	wal->bit = 0;
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
}

void
walker_visit(struct walker *wal, bit b)
{
	uintptr_t *chld;

	chld = node_from_tag(wal->cur)->chld;
	lrotate(chld+b, &wal->prev, &wal->cur);
	chld[b] = tag_back(chld[b]);

	wal->bit = b;
}

void
walker_next(struct walker *walk)
{
	struct node *node;

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
	struct node *cur_node;
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
	struct node *node = (void *)new;
	set_add_key(set, new, node->obj, len);
}

void
set_add_key(struct set *set, struct set_node *new,
            uint8_t *key, size_t len)
{
	struct walker walk[1];
	struct node *new_node;
	struct node *sib_node;

	if (!set) return;
	if (!new) return;

	new_node = (void *)new;

	if (!set->root) {
		set->root = tag_leaf(new_node);
		return;
	}

	walker_begin(walk, set);

	sib_node = node_antilocate(walk, key, len);
	if (!sib_node) goto done;

	node_attach(walk, new_node, sib_node, key, len);

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
	struct node *node;
	uintptr_t tag;
	bit b;
	
	if (!set) return false;
	if (!key) return false;

	tag = set->root;
	while (is_node(tag)) {
		node = node_from_tag(tag);
		b = bit_index_bytes(key, len, node->crit);
		if (b == 2) return false;
		tag = node->chld[b];
	}

	node = node_from_tag(tag);
	return byte_diff(node->obj, key, len) == -1UL;
}
