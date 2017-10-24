#ifndef _edna_frag_
#define _edna_frag_
#include <stddef.h>
#include <stdint.h>

#include <tag.h>

struct avl;

void      avl_add_chld(uintptr_t p, int k, uintptr_t c);
uintptr_t avl_adjust(uintptr_t t, int b);
int       avl_branch_of(uintptr_t t, uintptr_t p);
void      avl_delete(struct avl *del_node);
uintptr_t avl_detatch(uintptr_t p, int k);
void *    avl_get_root(struct avl *node);
uintptr_t avl_increment(uintptr_t t, int k);
void      avl_insert(struct avl *hint_node, int k, struct avl *new_node);
void      avl_free(struct avl *node);
uintptr_t avl_next(uintptr_t node, int direction);
uintptr_t avl_rotate(uintptr_t, int k);
uintptr_t avl_rotate2(uintptr_t, int k);
uintptr_t avl_swap(uintptr_t u, int k);
uintptr_t avl_tag_of(struct avl *);

static inline uintptr_t avl_chld_of(uintptr_t t, int k);
static inline uintptr_t avl_prnt_of(uintptr_t t);
static inline void      avl_set_link(uintptr_t u, int k, uintptr_t t);

struct avl {
	uintptr_t link[3];
};

#define untag_field(P, F) (((struct avl *)untag(P))->F)
uintptr_t avl_chld_of(uintptr_t t, int k) { return untag_field(t, link[k]); }
uintptr_t avl_prnt_of(uintptr_t t) { return untag_field(t, link[2]); }
void avl_link_to(uintptr_t u, int k, uintptr_t t) { untag_field(u, link[k])=t; }

#endif
