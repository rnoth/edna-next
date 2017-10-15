#include <unit.h>
#include <util.h>

#include <tag.h>
#include <frag.c>

static void test_adjust_branches(void);
static void test_adjust_leaves(void);
static void test_adjust_parent(void);
static void test_adjust_single(void);

static void test_delete_branch(void);
static void test_delete_leaf(void);
static void test_delete_root(void);

static void test_get_root(void);

static void test_increment_adjust(void);
static void test_increment_rotate(void);
static void test_increment_rotate2(void);

static void test_insert_balance_double(void);
static void test_insert_balance_single(void);
static void test_insert_balance_soft(void);
static void test_insert_empty(void);
static void test_insert_head(void);
static void test_insert_tail(void);

static void test_find_empty_chld(void);
static void test_find_empty_chld2(void);

static void test_rotate_left(void);
static void test_rotate_null(void);
static void test_rotate_right(void);

static void test_rotate2_left(void);
static void test_rotate2_null(void);
static void test_rotate2_right(void);

static void test_stab_absent(void);
static void test_stab_empty(void);
static void test_stab_root(void);

static void test_swap_succ(void);

struct unit_test tests[] = {
	{.msg = "should fail on stabbing an empty tree",
	 .fun = unit_list(test_stab_empty),},
	{.msg = "should insert a fragment to an empty tree",
	 .fun = unit_list(test_insert_empty),},

	{.msg = "should stab the root piece",
	 .fun = unit_list(test_stab_root),},
	{.msg = "should not stab node not in the tree",
	 .fun = unit_list(test_stab_absent),},

	{.msg = "should find suitable positions for inserting nodes",
	 .fun = unit_list(test_find_empty_chld),},
	{.msg = "should find suitable positions for inserting nodes "
	        "under branches",
	 .fun = unit_list(test_find_empty_chld2),},

	{.msg = "should prepend a fragment into a tree",
	 .fun = unit_list(test_insert_head),},
	{.msg = "should append a fragment into a tree",
	 .fun = unit_list(test_insert_tail),},

	{.msg = "should retrieve the root node",
	 .fun = unit_list(test_get_root),},

	{.msg = "should delete root nodes",
	 .fun = unit_list(test_delete_root),},

	{.msg = "should do nothing when adjust a node with no links",
	 .fun = unit_list(test_adjust_single),},
	{.msg = "should adjust the balance of nodes with leaf children",
	 .fun = unit_list(test_adjust_leaves),},
	{.msg = "should adjust the balance of nodes with branch children",
	 .fun = unit_list(test_adjust_branches),},
	{.msg = "should adjust the balance of node with parents",
	 .fun = unit_list(test_adjust_parent),},

	{.msg = "should update balance information on insert",
	 .fun = unit_list(test_insert_balance_soft),},
	{.msg = "should delete leaf nodes",
	 .fun = unit_list(test_delete_leaf),},

	{.msg = "should rotate to the left",
	 .fun = unit_list(test_rotate_left),},
	{.msg = "should rotate to the right",
	 .fun = unit_list(test_rotate_right),},
        {.msg = "should rotate two nodes",
         .fun = unit_list(test_rotate_null),},

	{.msg = "should double-rotate to the left",
	 .fun = unit_list(test_rotate2_left),},
	{.msg = "should double-rotate to the right",
	 .fun = unit_list(test_rotate2_right),},
	{.msg = "should double-rotate three nodes",
	 .fun = unit_list(test_rotate2_null),},

	{.msg = "should increment all pointers to a node",
	 .fun = unit_list(test_increment_adjust),},
	{.msg = "should rotate on balance tag overflow",
	 .fun = unit_list(test_increment_rotate),},
	{.msg = "should double rotate on balance tag overflow",
	 .fun = unit_list(test_increment_rotate2),},

	{.msg = "should rebalance the tree with single rotations on insertion",
	 .fun = unit_list(test_insert_balance_single),},
	{.msg = "should rebalance the tree with double rotations on insertion",
	 .fun = unit_list(test_insert_balance_double),},

	/* {.msg = "should swap nodes with their successor children", */
	/*  .fun = unit_list(test_swap_succ),}, */

	/* {.msg = "should delete branch nodes", */
	/*  .fun = unit_list(test_delete_branch),}, */
};

#include <unit.t>

#define expect_is_root(R) \
	okf(get_prnt(R) == 0, \
	    "expected %s to be a root node, instead has parent", \
	    #R); \

#define expect_is_leaf(L) \
	do { \
		okf(get_chld(L, 0) == 0, \
		    "expected %s to be a leaf node, instead has left child", \
		    #L); \
		okf(get_chld(L, 1) == 0, \
		    "expected %s to be a leaf node, instead has right child", \
		    #L); \
	} while (0)

#define expect_has_chld(T, K, C) \
	do { \
		char *dir = K ? "right" : "left"; \
		okf(untag(get_chld(T, K)) == untag(C), \
		    "expected %s to have %s child %s", \
		    #T, dir, #C); \
		if (C) \
			okf(untag(get_prnt(C)) == untag(T), \
			    "expected %s to have parent %s", \
			    #C, #T); \
		okf(get_chld(T, K) == C, \
		   "expected %s to have %s chld with tag %d, instead of %d", \
		   #T, dir, tag_of(C), tag_of(get_chld(T, K))); \
		if (C) \
			okf(get_prnt(C) == T, \
			    "expected %s to have parent with tag %d, instead of %d", \
			    #C, tag_of(T), tag_of(get_prnt(C))); \
	} while (0)

uintptr_t
make_tree(size_t n, int b, uintptr_t l, uintptr_t r)
{
	struct frag *d;
	uintptr_t u;
	uintptr_t i;

	ok(d = calloc(1, sizeof *d));
	u = tag0(d) + (b ? 2|b>0 : 0);
	d->len = n;
	d->off[1] = n;

	if (l) {
		d->off[0] += get_off(l, 1);
		add_chld(u, 0, l);
	}

	if (r) {
		i = r;
		do {
			get_field(i, off[0]) += n;
			get_field(i, off[1]) += n;
		} while (i = get_chld(i, 0));

		d->off[1] = d->off[0] + get_off(r, 1);

		add_chld(u, 1, r);
	}

	return u;
}

void
kill_tree(uintptr_t t)
{
	if (!t) return;
	kill_tree(get_chld(t, 0));
	kill_tree(get_chld(t, 1));
	free(untag(t));
}

void
test_adjust_branches(void)
{
	uintptr_t a, b, c, d, e;

	b = make_tree(2, 0, 0, 0);
	d = make_tree(4, 0, 0, 0);

	a = make_tree(1, 1, 0, b);
	e = make_tree(5, -1, d, 0);

	c = make_tree(3, 0, a, e);

	try(adjust(c, 3));

	expect_has_chld(c + 3, 0, a);
	expect_has_chld(c + 3, 1, e);

	kill_tree(c);
}

void
test_adjust_leaves(void)
{
	uintptr_t a, b, c;

	b = make_tree(2, 0, a = make_tree(1, 0, 0, 0),
	                    c = make_tree(3, 0, 0, 0));

	try(adjust(b, 3));

	expect_has_chld(b + 3, 0, a);
	expect_has_chld(b + 3, 1, c);

	kill_tree(b);
}

void
test_adjust_parent(void)
{
	uintptr_t a, b;

	b = make_tree(2, 0, a = make_tree(1, 0, 0, 0), 0);

	try(adjust(a, 3));
	expect_has_chld(b, 0, a + 3);

	kill_tree(b);
}

void
test_adjust_single(void)
{
	uintptr_t a;

	a = make_tree(4, 0, 0, 0);

	try(frag_insert(0, 0, untag(a)));

	try(adjust(a, 0));

	kill_tree(a);
}

void
test_delete_branch(void)
{
	uintptr_t a, b, c;

	a = make_tree(1, 0,0,0);
	c = make_tree(4, 0,0,0);
	b = make_tree(2, 0,a,c);

	try(frag_delete(untag(b)));

	expect_is_root(c);
	expect_is_leaf(a);
	expect_has_chld(c ^ 2, 0, a);
	expect_has_chld(c, 1, 0);

	expect(0, get_off(a, 0));
	expect(1, get_off(a, 1));
	expect(3, get_off(c, 0));
	expect(7, get_off(c, 1));

	ok(frag_stab(untag(c), 0) == untag(a));
	ok(frag_stab(untag(c), 1) == untag(c));
	ok(frag_stab(untag(c), 2) == untag(c));
	ok(frag_stab(untag(c), 3) == untag(c));

}

void
test_delete_leaf(void)
{
	uintptr_t a, b;

	b = make_tree(7, 0, 0, 0);
	a = make_tree(3, 1, 0, b);

	try(frag_delete(untag(b)));

	expect_is_leaf(a);
	expect_is_root(a);

	expect(0, get_off(a, 0));
	expect(3, get_off(a, 1));
}

void
test_delete_root(void)
{
	struct frag r[1] = {{.len = 4}};

	try(frag_insert(0, 0, r));

	try(frag_delete(r));
}

void
test_get_root(void)
{
	struct frag a[1] = {{.len = 10}};
	struct frag b[1] = {{.len = 5}};

	try(frag_insert(0, 0, a));
	try(frag_insert(a, 10, b));

	ok(a == frag_get_root(a));
	ok(a == frag_get_root(b));
}

void
test_increment_adjust(void)
{
	uintptr_t a, b, c;

	b = make_tree(2, -1, a = make_tree(1, 0, 0, 0),
	                     c = make_tree(3, 0, 0, 0));

	try(increment(b, 1));

	expect_has_chld(b ^ 2, 0, a);
	expect_has_chld(b ^ 2, 1, c);
}

void
test_increment_rotate(void)
{
	uintptr_t a, b, c;

	a = make_tree(1, -1, b = make_tree(2, -1, c = make_tree(3,0,0,0),0),0);

	try(increment(a, 0));

	expect_is_leaf(a);

	expect_is_leaf(c);

	expect_has_chld(b ^ 2, 0, c);
	expect_is_root(b);
}

void
test_increment_rotate2(void)
{
	uintptr_t a, b, c;

	a = make_tree(1, 1, 0, c = make_tree(3, -1, b = make_tree(2,0,0,0),0));

	try(increment(a, 1));

	expect_is_leaf(a);
	expect_is_leaf(c);
	expect_has_chld(b, 0, a ^ 3);
	expect_has_chld(b, 1, c ^ 2);
	expect_is_root(b);
}

void
test_insert_empty(void)
{
	struct frag a[1]={{.len=14}};

	try(frag_insert(0, 0, a));

	expect_is_leaf(tag0(a));
}

void
test_insert_balance_double(void)
{
	uintptr_t a, b, c;

	c = make_tree(3,0,0,0);
	a = make_tree(1,1,0,c);
	b = make_tree(2,0,0,0);

	try(frag_insert(untag(a), 1, untag(b)));

	expect_has_chld(b, 0, a ^ 3);
	expect_has_chld(b, 1, c);

	expect_is_root(b);

	expect_is_leaf(a);
	expect_is_leaf(c);
}

void
test_insert_balance_single(void)
{
	uintptr_t a, b, c;

	c = make_tree(4, 0, 0, 0);
	b = make_tree(2, 0, 0, 0);
	a = make_tree(1, 1, 0, b);

	try(frag_insert(untag(a), 3, untag(c)));

	expect_is_leaf(c);
	expect_is_leaf(a);

	expect_has_chld(b, 0, a ^ 3);
	expect_has_chld(b, 1, c);
	expect_is_root(b);
}

void
test_insert_balance_soft(void)
{
	uintptr_t a, b, c;

	a = make_tree(1,  0, 0, 0);
	b = make_tree(2, -1, a, 0);
	c = make_tree(3,  0, 0, 0);

	try(frag_insert(untag(b), 3, untag(c)));

	expect_is_root(b);
	expect_is_leaf(c);
	expect_has_chld(b ^ 2, 1, c);
}

void
test_insert_head(void)
{
	uintptr_t a, b;

	a = make_tree(4, 0, 0, 0);
	b = make_tree(6, 0, 0, 0);

	try(frag_insert(untag(a), 0, untag(b)));

	expect_is_leaf(b);
	expect_is_root(a);
	expect_has_chld(a^2, 0, b);

	expect(6, get_off(a, 0));
	expect(10, get_off(a, 1));

	kill_tree(a);
}

void
test_insert_tail(void)
{
	uintptr_t a, b;

	a = make_tree(4, 0, 0, 0);
	b = make_tree(6, 0, 0, 0);

	try(frag_insert(untag(a), 4, untag(b)));

	expect_is_leaf(b);
	expect_is_root(a);
	expect_has_chld(a^2, 1, b);

	expect(0, get_off(a, 0));
	expect(10, get_off(a, 1));

	expect(4, get_off(b, 0));

	kill_tree(a);
}

void
test_find_empty_chld(void)
{
	uintptr_t a;
	uintptr_t t;

	t = a = make_tree(10, 0, 0, 0);

	ok(find_empty_chld(&t, 0) == 0);
	ok(t == a);

	ok(find_empty_chld(&t, 10) == 1);
	ok(t == a);
}

void
test_find_empty_chld2(void)
{
	uintptr_t a, b;
	uintptr_t t;

	b = make_tree(5,0,0,0);
	t = a = make_tree(10,0,0,b);

	ok(find_empty_chld(&t, 0) == 0);
	ok(t == a);

	ok(find_empty_chld(&t, 10) == 0);
	ok(t == b);

	t=a;
	ok(find_empty_chld(&t, 15) == 1);
	ok(t == b);
}

void
test_rotate_left(void)
{
	uintptr_t a, b;
	uintptr_t f, g, h;

	h = make_tree(16, 0, 0, 0);
	g = make_tree( 8, 0, 0, 0);
	f = make_tree( 4, 0, 0, 0);
	b = make_tree( 2, 0, g, h);
	a = make_tree( 1, 1, f, b);

	ok(rotate(a, 0) == b);

	expect_has_chld(a, 0, f);
	expect_has_chld(a, 1, g);

	expect_has_chld(b, 0, a);
	expect_has_chld(b, 1, h);

	expect_is_root(b);

	expect( 4, get_off(a, 0));
	expect(13, get_off(a, 1));
	expect(13, get_off(b, 0));
	expect(31, get_off(b, 1));

	kill_tree(b);
}

void
test_rotate_null(void)
{
	uintptr_t a, b;

	a = make_tree(1, 1, 0, b = make_tree(2, 0, 0, 0));
	ok(rotate(a, 0) == b);

	expect_is_root(b);
	expect_is_leaf(a);
	expect_has_chld(b, 0, a);
}

void
test_rotate_right(void)
{
	uintptr_t a, b;
	uintptr_t f, g, h;

	h = make_tree(16, 0, 0, 0);
	g = make_tree( 8, 0, 0, 0);
	f = make_tree( 4, 0, 0, 0);
	b = make_tree( 2, 0, f, g);
	a = make_tree( 1,-1, b, h);

	ok(rotate(a, 1) == b);

	expect_has_chld(a, 0, g);
	expect_has_chld(a, 1, h);

	expect_has_chld(b, 0, f);
	expect_has_chld(b, 1, a);

	expect_is_root(b);

	expect(10, get_off(a, 0));
	expect(27, get_off(a, 1));
	expect( 4, get_off(b, 0));
	expect(31, get_off(b, 1));
}

void
test_rotate2_left(void)
{
	uintptr_t a, b, c;
	uintptr_t e, f, g, h;

	a = make_tree(1, 1,
	              e = make_tree(2,0,0,0),
	              b = make_tree(4, -1,
	                            c = make_tree(8, 0,
	                                        f = make_tree(16,0,0,0),
	                                        g = make_tree(32,0,0,0)),
	                            h = make_tree(64,0,0,0)));

	ok(rotate2(a, 0) == c);

	ok(get_chld(a, 0) == e);
	ok(get_chld(a, 1) == f);
	ok(get_prnt(a) == c);

	ok(get_chld(b, 0) == g);
	ok(get_chld(b, 1) == h);
	ok(get_prnt(b) == c);

	ok(get_chld(c, 0) == a);
	ok(get_chld(c, 1) == b);
	ok(get_prnt(c) == 0);

	expect(2, get_off(a, 0));
	expect(19, get_off(a, 1));

	expect(40, get_off(b, 0));
	expect(108, get_off(b, 1));

	expect(19, get_off(c, 0));
	expect(127, get_off(c, 1));

	kill_tree(b);
}

void
test_rotate2_null(void)
{
	uintptr_t a, b, c;

	a = make_tree(1, 1,
	              0,
	              c = make_tree(3, -1,
	                            b = make_tree(2, 0, 0, 0),
	                            0));

	ok(rotate2(a, 0) == b);

	expect_has_chld(b, 0, a);
	expect_has_chld(b, 1, c);
	expect_is_root(b);

	kill_tree(b);
}

void
test_rotate2_right(void)
{
	uintptr_t a, b, c;
	uintptr_t e, f, g, h;

	b=make_tree(1, -1,
	            a = make_tree(2, 1,
	                          e = make_tree(4,0,0,0),
	                          c = make_tree(8, 0,
	                                    f = make_tree(16,0,0,0),
	                                    g = make_tree(32,0,0,0))),
	            h = make_tree(64,0,0,0));

	ok(rotate2(b, 1) == c);

	expect_has_chld(a, 0, e);
	expect_has_chld(a, 1, f);

	expect_has_chld(b, 0, g);
	expect_has_chld(b, 1, h);

	expect_has_chld(c, 0, a);
	expect_has_chld(c, 1, b);

	expect_is_root(c);

	expect(4, get_off(a, 0));
	expect(22, get_off(a, 1));

	expect(40, get_off(b, 0));
	expect(105, get_off(b, 1));

	expect(22, get_off(c, 0));
	expect(127, get_off(c, 1));

	kill_tree(b);
}

void
test_stab_absent(void)
{
	struct frag root[1]={{.len = 10}};

	try(frag_insert(0, 0, root));
	ok(!frag_stab(root, 11));
}

void
test_stab_empty(void)
{
	ok(!frag_stab(0, 5));
}

void
test_stab_root(void)
{
	struct frag r[1]={{.len = 10}};

	try(frag_insert(0, 0, r));
	ok(frag_stab(r, 0));
	ok(frag_stab(r, 4));
	ok(frag_stab(r, 0) == r);
	ok(frag_stab(r, 4) == r);
}

void
test_swap_succ(void)
{
	uintptr_t f, g;
	uintptr_t a, b;

	f = make_tree(4, 0, 0, 0);
	g = make_tree(8, 0, 0, 0);

	b = make_tree(2,-1, 0, g);
	a = make_tree(1, 0, f, b);

	try(swap(a, 1));

	expect_has_chld(a, 0, 0);
	expect_has_chld(a, 1, g);

	expect_has_chld(b, 0, f);
	expect_has_chld(b, 1, a);
	expect_is_root(b);

	expect(0, get_off(a, 0));
	expect(20, get_off(a, 1));

	expect(10, get_off(b, 0));
	expect(21, get_off(b, 1));
}
