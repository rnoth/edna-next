#ifndef _edna_tag_
#define _edna_tag_

#include <stdbool.h>
#include <stdint.h>

static inline uintptr_t flip_tag(uintptr_t);

static inline bool is_back(uintptr_t);
static inline bool is_node(uintptr_t);
static inline bool is_leaf(uintptr_t);
static inline bool is_root(uintptr_t);

static inline uintptr_t tag_node(void *);
static inline uintptr_t tag_leaf(void *);
static inline uintptr_t tag_root(void *);

static inline uintptr_t tag0(void *);
static inline uintptr_t tag1(void *);
static inline uintptr_t tag2(void *);
static inline uintptr_t tag3(void *);

static inline int tag_of(uintptr_t);
static inline void *untag(uintptr_t);

uintptr_t flip_tag(uintptr_t tag) { return tag ^ 2; }

bool is_back(uintptr_t tag) { return !!(tag & 2); }
bool is_leaf(uintptr_t tag) { return tag & 1; }
bool is_node(uintptr_t tag) { return !(tag & 1); }
bool is_root(uintptr_t tag) { return tag & 1; }

int tag_of(uintptr_t tag) { return tag & 3; }
void *untag(uintptr_t tag) { return (void *)(tag & ~3); }

uintptr_t tag_leaf(void *leaf) { return (uintptr_t)leaf | 1; }
uintptr_t tag_node(void *node) { return (uintptr_t)node; }
uintptr_t tag_root(void *root) { return (uintptr_t)root | 1; }

uintptr_t tag0(void *ptr) { return (uintptr_t)ptr; }
uintptr_t tag1(void *ptr) { return (uintptr_t)ptr | 1; }
uintptr_t tag2(void *ptr) { return (uintptr_t)ptr | 2; }
uintptr_t tag3(void *ptr) { return (uintptr_t)ptr | 3; }

#endif
