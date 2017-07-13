#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <util.h>
#include <set.h>

#define node_of(n) ((struct node *)((char *)(n) - offsetof(struct node,obj)))

#define tag_back(u) ((u) | 4)
#define tag_leaf(n) ((uintptr_t)(n) | 1)
#define tag_node(n) ((uintptr_t)(n))
#define tag_set(s) ((uintptr_t)s | 2)

#define tag_from_back(u) (u ^ 4)
#define node_from_tag(u) ((struct node *)(u & ~1))
#define set_from_tag(u) ((struct set *)(u & ~2))

#define is_back(u) !!(u & 4)
#define is_leaf(u) (u & 1)
#define is_node(u) !(u & 3)
#define is_set(u) !!(u & 2)

typedef int8_t bit;

struct node {
	uintptr_t chld[2];
	size_t    crit;
	size_t    size;
	uint8_t   obj[];
};

struct walker {
	uintptr_t cur;
	uintptr_t prev;
	uint8_t   ord:3;
	uint8_t   bit:1;
};

static void attach_node(struct walker *, struct node *, struct node *);
static bit bit_index_bytes(uint8_t *, size_t, size_t);
static bit bit_index_node(struct node *, size_t);
static struct node *node_antilocate_match(struct walker *, struct node *);
static bool nodes_are_matches(struct node *, struct node *);
static size_t node_diff(struct node *, struct node *);
static bool node_matches_key(struct node *, uint8_t *, size_t);
static void walker_begin(struct walker *, struct set *);
static void walker_finish(struct walker *);
static void walker_next(struct walker *);
static void walker_rise(struct walker *);
static void walker_visit(struct walker *, bit);
static void walker_walk(struct walker *, void *, size_t);

void
attach_node(struct walker *wal, struct node *el_node, struct node *sib_node)
{
	struct node *cur_node;
	struct set *set;
	uintptr_t *dest_tag_ptr;
	bit b;
	
	el_node->crit = node_diff(el_node, sib_node);
	b = bit_index_node(el_node, el_node->crit);
	el_node->chld[b] = tag_leaf(el_node);
	
	while (walker_rise(wal), !is_set(wal->cur)) {

		cur_node = node_from_tag(wal->cur);
		dest_tag_ptr = cur_node->chld + wal->bit;
		if (cur_node->crit < el_node->crit) goto attach;
	}

	set = set_from_tag(wal->cur);
	dest_tag_ptr = &set->root;

 attach:
	b = !el_node->chld[1];
	el_node->chld[b] = *dest_tag_ptr;
	*dest_tag_ptr = tag_node(el_node);
}

bit
bit_index_bytes(uint8_t *key, size_t len, size_t crit)
{
	size_t byt = crit / 8;
	size_t bit = crit % 8;

	if (byt >= len) return 2;

	return !!(key[byt] & 128 >> bit);
}

bit
bit_index_node(struct node *node, size_t crit)
{
	return bit_index_bytes(node->obj, node->size, crit);
}

struct node *
node_antilocate_match(struct walker *wal, struct node *key_node)
{
	struct node *match_node;

	walker_walk(wal, key_node->obj, key_node->size);
	match_node = node_from_tag(wal->cur);

	if (!nodes_are_matches(key_node, match_node)) {
		return match_node;
	} else return 0;
}

bool
nodes_are_matches(struct node *lef_node, struct node *rit_node)
{
	return node_matches_key(lef_node, rit_node->obj, rit_node->size);
}

size_t
node_diff(struct node *lef_node, struct node *rit_node)
{
	size_t pos = 0;
	uint8_t off = 0;
	uint8_t diff;
	
	while (lef_node->obj[pos] == rit_node->obj[pos]) ++pos;

	diff = lef_node->obj[pos] ^ rit_node->obj[pos];
	
	if (diff & 0xf0) diff >>= 4, off |= 4;
	if (diff & 0x0c) diff >>= 2, off |= 2;
	if (diff & 0x02) diff >>= 1, off |= 1;

	return pos * 8 + 7 - off;
}

bool
node_matches_key(struct node *node, uint8_t *key, size_t len)
{
	size_t min_size;

	min_size = umin(node->size, len);
	return !memcmp(node->obj, key, min_size);
}

void
walker_begin(struct walker *wal, struct set *set)
{
	wal->prev = tag_set(set);
	wal->cur = set->root;
	wal->ord = 0;
	wal->bit = 0;
}

void
walker_finish(struct walker *wal)
{
	while (!is_set(wal->cur)) {
		walker_rise(wal);
	}
}

void
walker_rise(struct walker *wal)
{
	struct node *cur_node;
	struct set *set;
	uintptr_t *cur_chld;
	uintptr_t temp_tag;
	bit b;

	if (is_set(wal->prev)) {
		temp_tag = wal->prev;
		set = set_from_tag(wal->prev);
		wal->prev = 0;
		set->root = wal->cur;
		wal->cur = temp_tag;
		wal->prev = 0;
		wal->ord = 1;
		return;
	}
	
	cur_node = node_from_tag(wal->prev);
	cur_chld = cur_node->chld;
	b = is_back(cur_chld[1]);
	temp_tag = wal->prev;
       	wal->prev = tag_from_back(cur_chld[b]);
	cur_chld[b] = wal->cur;
	wal->cur = temp_tag;

	wal->ord = 1 + b;
	wal->bit = b;
}

void
walker_visit(struct walker *wal, bit b)
{
	struct node *cur_node;
	uintptr_t *cur_chld;
	uintptr_t temp_tag;

	cur_node = node_from_tag(wal->cur);
	cur_chld = cur_node->chld;
	temp_tag = wal->cur;
	wal->cur = cur_chld[b];
	cur_chld[b] = tag_back(wal->prev);
	wal->prev = temp_tag;

	wal->ord = 0;
	wal->bit = b;
}

void
walker_next(struct walker *wal)
{
	switch (wal->ord) {
	case 0:
		if (is_leaf(wal->cur)) goto rise;
		walker_visit(wal, 0);
		break;
	case 1:
		if (is_leaf(wal->cur)) goto rise;
		walker_visit(wal, 1);
		break;
		
	case 2:
	rise:
		walker_rise(wal);
		break;
	}
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

void *
set_alloc(size_t len)
{
	struct node *res_node;

	res_node = calloc(1, sizeof *res_node + len);
	if (!res_node) return 0;

	res_node->size = len;

	return res_node->obj;
}

void
set_add(struct set *set, void *el)
{
	struct walker wal[1];
	struct node *sib_node;
	struct node *el_node;

	if (!set) return;
	if (!el)  return;

	el_node = node_of(el);

	if (!set->root) {
		set->root = tag_leaf(el_node);
		return;
	}

	walker_begin(wal, set);

	sib_node = node_antilocate_match(wal, el_node);
	if (!sib_node) return;

	attach_node(wal, el_node, sib_node);

	walker_finish(wal);
}

void
set_free(struct set *set)
{
	struct walker wal[1] = {{0}};
	struct node *cur_node;
	
	walker_begin(wal, set);

	while (!is_set(wal->cur)) {
		cur_node = node_from_tag(wal->cur);
		
		if (wal->ord == 2) {
			free(cur_node);
		} else if (!cur_node->chld[1]) {
			free(cur_node);
		}

		walker_next(wal);
	}
}

void *
set_rm(struct set *set, void *key, size_t len)
{
	return 0;
}

bool
set_has(struct set *set, void *key, size_t len)
{
	struct node *cur_node;
	uintptr_t cur_tag;
	bit b;
	
	if (!set) return false;
	if (!key) return false;

	cur_tag = set->root;
	while (is_node(cur_tag)) {
		cur_node = node_from_tag(cur_tag);
		b = bit_index_bytes(key, len, cur_node->crit);
		if (b == 2) return false;
		cur_tag = cur_node->chld[b];
	}

	cur_node = node_from_tag(cur_tag);
	return node_matches_key(cur_node, key, len);
}

char *
set_str(char *str)
{
	char *ret;

	ret = set_alloc(strlen(str) + 1);
	if (!ret) return 0;

	strcpy(ret, str);

	return ret;
}
