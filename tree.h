#ifndef _edna_tree_
#define _edna_tree_

#include <tag.h>

static inline bool is_back(uintptr_t);
static inline bool is_node(uintptr_t);
static inline bool is_leaf(uintptr_t);
static inline bool is_root(uintptr_t);

static inline uintptr_t tag_node(void *);
static inline uintptr_t tag_leaf(void *);
static inline uintptr_t tag_root(void *);

static inline uintptr_t flip_tag(uintptr_t);

uintptr_t flip_tag(uintptr_t tag) { return tag ^ 2; }

bool is_back(uintptr_t tag) { return !!(tag & 2); }
bool is_leaf(uintptr_t tag) { return tag & 1; }
bool is_node(uintptr_t tag) { return !(tag & 1); }
bool is_root(uintptr_t tag) { return tag & 1; }

uintptr_t tag_leaf(void *leaf) { return (uintptr_t)leaf | 1; }
uintptr_t tag_node(void *node) { return (uintptr_t)node; }
uintptr_t tag_root(void *root) { return (uintptr_t)root | 1; }

#endif
