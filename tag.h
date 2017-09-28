#ifndef _edna_tag_
#define _edna_tag_

#include <stdbool.h>
#include <stdint.h>

static inline uintptr_t tag0(void *);
static inline uintptr_t tag1(void *);
static inline uintptr_t tag2(void *);
static inline uintptr_t tag3(void *);

static inline int tag_of(uintptr_t);
static inline void *untag(uintptr_t);

int tag_of(uintptr_t tag) { return tag & 3; }
void *untag(uintptr_t tag) { return (void *)(tag & ~3); }

uintptr_t tag0(void *ptr) { return (uintptr_t)ptr; }
uintptr_t tag1(void *ptr) { return (uintptr_t)ptr | 1; }
uintptr_t tag2(void *ptr) { return (uintptr_t)ptr | 2; }
uintptr_t tag3(void *ptr) { return (uintptr_t)ptr | 3; }

#endif
