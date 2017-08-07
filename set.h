#ifndef _edna_set_
#define _edna_set_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct set;
struct set_node;
struct set_context;

void set_add(struct set *set, struct set_node *new, size_t length);
bool set_has(struct set *set, void *key, size_t length);

void set_add_key(struct set *set, struct set_node *new,
                 uint8_t *key, size_t length);
void set_has_iter(struct set *set, struct set_context *ctx,
                  void *key, size_t length);
void *set_has_cont(struct set_context *ctx, void *val, size_t length);

struct set {
	uintptr_t root;
	size_t height;
};

struct set_node {
	uintptr_t chld[2];
	size_t    crit;
};

#endif
