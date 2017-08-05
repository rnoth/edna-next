#ifndef _edna_set_
#define _edna_set_
#include <stdbool.h>

#define set_ctor(OBJ_PTR) (*(OBJ_PTR) = set_alloc(sizeof **(OBJ_PTR)))
#define set_new(TYPE) (set_alloc(sizeof (TYPE)))

struct set {
	uintptr_t root;
	size_t height;
};

void *set_alloc(size_t);
void set_add(struct set *, void *);
void set_free(struct set *);
bool set_has(struct set *, void *, size_t);
char *set_str(char *);

#endif
