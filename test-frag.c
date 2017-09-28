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

static void test_finger(void);
static void test_flush_empty(void);
static void test_flush_idempotent(void);
static void test_flush_one(void);
static void test_flush_two(void);
static void test_flush_offset(void);

static void test_increment_adjust(void);
static void test_increment_rotate(void);
static void test_increment_rotate2(void);

static void test_insert_balance_double(void);
static void test_insert_balance_single(void);
static void test_insert_balance_soft(void);
static void test_insert_empty(void);
static void test_insert_head(void);
static void test_insert_tail(void);

static void test_rebalance_expand_two(void);
static void test_rebalance_expand_single(void);

static void test_rotate_left(void);
static void test_rotate_null(void);
static void test_rotate_right(void);

static void test_rotate2_left(void);
static void test_rotate2_null(void);
static void test_rotate2_right(void);

static void test_stab_absent(void);
static void test_stab_empty(void);
static void test_stab_nearest(void);
static void test_stab_root(void);

static void test_swap_succ(void);

struct unit_test tests[] = {
	{.msg = "should fail on stabbing an empty tree",
	 .fun = unit_list(test_stab_empty),},
	{.msg = "should insert a fragment to an empty tree",
	 .fun = unit_list(test_insert_empty),},

	{.msg = "should stay at nearest node in the tree after stabbing",
	 .fun = unit_list(test_stab_nearest),},

	{.msg = "should stab the root piece",
	 .fun = unit_list(test_stab_root),},
	{.msg = "should not stab node not in the tree",
	 .fun = unit_list(test_stab_absent),},

	{.msg = "should prepend a fragment into a tree",
	 .fun = unit_list(test_insert_head),},
	{.msg = "should append a fragment into a tree",
	 .fun = unit_list(test_insert_tail),},
	{.msg = "should update balance information on insert",
	 .fun = unit_list(test_insert_balance_soft),},

	{.msg = "should maintain last query location",
	 .fun = unit_list(test_finger),},

	{.msg = "should do nothing when flushing an empty tree",
	 .fun = unit_list(test_flush_empty),},
	{.msg = "should do nothing when flushing a one-element tree",
	 .fun = unit_list(test_flush_one),},
	{.msg = "should point at the root after flushing a two-element tree",
	 .fun = unit_list(test_flush_two),},
	{.msg = "should do nothing when flushing with the finger at the root",
	 .fun = unit_list(test_flush_idempotent),},
	{.msg = "should reset the displacement after flushing a tree",
	 .fun = unit_list(test_flush_offset),},

	{.msg = "should delete root nodes",
	 .fun = unit_list(test_delete_root),},
	{.msg = "should delete leaf nodes",
	 .fun = unit_list(test_delete_leaf),},

	{.msg = "should do nothing when adjust a node with no links",
	 .fun = unit_list(test_adjust_single),},
	{.msg = "should adjust the balance of nodes with leaf children",
	 .fun = unit_list(test_adjust_leaves),},
	{.msg = "should adjust the balance of nodes with branch children",
	 .fun = unit_list(test_adjust_branches),},
	{.msg = "should adjust the balance of node with parents",
	 .fun = unit_list(test_adjust_parent),},

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

	{.msg = "should rebalance on expansion with two nodes",
	 .fun = unit_list(test_rebalance_expand_two),},
	{.msg = "should rebalance on expansion with threes (single case)",
	 .fun = unit_list(test_rebalance_expand_single),},

	{.msg = "should rebalance the tree with single rotations on insertion",
	 .fun = unit_list(test_insert_balance_single),},
	{.msg = "should rebalance the tree with double rotations on insertion",
	 .fun = unit_list(test_insert_balance_double),},

	{.msg = "should swap nodes with their successor children",
	 .fun = unit_list(test_swap_succ),},

	{.msg = "should delete branch nodes",
	 .fun = unit_list(test_delete_branch),},
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
		okf(get_off(L, 0) == 0, \
		    "expect leaf %s to have no offset, but has offset %zu", \
		    #L, get_off(L, 0)); \
		okf(get_off(L, 1) == 0, \
		    "expected leaf %s to have no extent, but has extent %zu", \
		    #L, get_off(L, 1)); \
	} while (0)

#define expect_has_chld(T, K, C) \
	do { \
		char *dir = K ? "right" : "left"; \
		okf(untag(get_chld(T, K)) == untag(C), \
		    "expected %s to have %s child %s", \
		    #T, dir, #C); \
		okf(untag(get_prnt(C)) == untag(T), \
		   "expected %s to have parent %s", \
		   #C, #T); \
		okf(get_chld(T, K) == C, \
		   "expected %s to have %s chld with tag %d, instead of %d", \
		   #T, dir, tag_of(C), tag_of(get_chld(T, K))); \
		okf(get_prnt(C) == T, \
		   "expected %s to have parent with tag %d, instead of %d", \
		   #C, tag_of(T), tag_of(get_prnt(C))); \
	} while (0)

uintptr_t
make_tree(size_t n, int w, uintptr_t l, uintptr_t r)
{
	struct frag_node *d;
	uintptr_t u;

	ok(d = calloc(1, sizeof *d));
	u = tag0(d) + (w ? 2|w>0 : 0);
	d->len = n;

	if (l) add_chld(u, 0, l);
	if (r) add_chld(u, 1, r);

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

	try(adjust_balance(c, 3));

	expect_has_chld(c + 3, 0, a);
	expect_has_chld(c + 3, 1, e);

	try(kill_tree(c));
}

void
test_adjust_leaves(void)
{
	uintptr_t a, b, c;

	b = make_tree(2, 0, a = make_tree(1, 0, 0, 0),
	                    c = make_tree(3, 0, 0, 0));

	try(adjust_balance(b, 3));

	expect_has_chld(b + 3, 0, a);
	expect_has_chld(b + 3, 1, c);

	kill_tree(c);
}

void
test_adjust_parent(void)
{
	uintptr_t a, b;

	b = make_tree(2, 0, a = make_tree(1, 0, 0, 0), 0);

	try(adjust_balance(a, 3));
	expect_has_chld(b, 0, a + 3);

	kill_tree(b);
}

void
test_adjust_single(void)
{
	struct frag_node a[1]={{.len=4}};
	struct frag fg[1] = {{0}};

	try(frag_insert(fg, 0, a));

	try(adjust_balance(fg->cur, 0));
}

void
test_delete_branch(void)
{
	struct frag t[1] = {{0}};
	uintptr_t a, b, c;

	a = make_tree(1, 0,0,0);
	c = make_tree(3, 0,0,0);
	b = make_tree(2, 0,a,c);

	t->cur = b;

	try(frag_delete(t, untag(b)));

	expect_is_root(a);
	expect_is_leaf(c);
	expect_has_chld(a, 1, c);
}

void
test_delete_leaf(void)
{
	struct frag t[1]={{1}};
	uintptr_t a, b;

	b = make_tree(7, 0, 0, 0);
	a = make_tree(3, 1, 0, b);

	try(frag_delete(t, untag(b)));

	expect_is_leaf(a);
	expect_is_root(a);
}

void
test_delete_root(void)
{
	struct frag_node root[1] = {{.len = 4}};
	struct frag fg[1] = {{0}};

	expect(0, frag_insert(fg, 0, root));

	try(frag_delete(fg, root));
	ok(!frag_stab(fg, 1));
}

void
test_finger(void)
{
	struct frag_node one[1] = {{.len = 4}};
	struct frag_node two[1] = {{.len = 6}};
	struct frag fg[1] = {{0}};

	expect(0, frag_insert(fg, 0, one));
	expect(0, frag_insert(fg, 0, two));

	ok(untag(fg->cur) == two);
	ok(untag(two->link[2]) == one);
}

void
test_flush_empty(void)
{
	struct frag fg[1] = {{0}};

	try(frag_flush(fg));

	ok(!fg->cur);
	ok(!fg->off);
}

void
test_flush_idempotent(void)
{
	struct frag_node one[1] = {{.len = 10}};
	struct frag_node two[1] = {{.len = 5}};
	struct frag fg[1] = {{0}};

	expect(0, frag_insert(fg, 0, one));
	expect(0, frag_insert(fg, 10, two));

	try(frag_flush(fg));

	ok(untag(fg->cur) == one);
	ok(!fg->off);

	try(frag_flush(fg));

	ok(untag(fg->cur) == one);
	ok(!fg->off);
}

void
test_flush_one(void)
{
	struct frag_node one[1] = {{.len = 2}};
	struct frag fg[1] = {{0}};

	expect(0, frag_insert(fg, 0, one));

	try(frag_flush(fg));

	ok(untag(fg->cur) == one);
	ok(!fg->off);
}

void
test_flush_two(void)
{
	struct frag_node one[1] = {{.len = 4}};
	struct frag_node two[1] = {{.len = 6}};
	struct frag fg[1] = {{0}};

	try(frag_insert(fg, 0, one));
	try(frag_insert(fg, 0, two));

	try(frag_flush(fg));

	ok(untag(fg->cur) == one);
	ok(!fg->off);
}

void
test_flush_offset(void)
{
	struct frag_node one[1] = {{.len = 4}};
	struct frag_node two[1] = {{.len = 6}};
	struct frag fg[1] = {{0}};

	try(frag_insert(fg, 0, one));
	try(frag_insert(fg, 4, two));

	try(frag_flush(fg));

	ok(untag(fg->cur) == one);
	ok(!fg->off);
}

void
test_increment_adjust(void)
{
	uintptr_t a, b, c;

	b = make_tree(2, -1, a = make_tree(1, 0, 0, 0),
	                     c = make_tree(3, 0, 0, 0));

	try(increment_chld(b, 1));

	expect_has_chld(b ^ 2, 0, a);
	expect_has_chld(b ^ 2, 1, c);
}

void
test_increment_rotate(void)
{
	uintptr_t a, b, c;

	a = make_tree(1, -1, b = make_tree(2, -1, c = make_tree(3,0,0,0),0),0);

	try(increment_chld(a, 0));

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

	try(increment_chld(a, 1));

	expect_is_leaf(a);
	expect_is_leaf(c);
	expect_has_chld(b, 0, a ^ 3);
	expect_has_chld(b, 1, c ^ 2);
	expect_is_root(b);
}

void
test_insert_empty(void)
{
	struct frag_node a[1]={{.len=14}};
	struct frag t[1] = {{0}};

	expect(0, frag_insert(t, 0, a));

	ok(untag(t->cur) == a);
	expect_is_leaf(tag0(a));
}

void
test_insert_balance_double(void)
{
	struct frag t[1]={{0}};
	uintptr_t a, b, c;

	a = make_tree(1, 1, 0, c = make_tree(3,0,0,0));
	b = make_tree(2, 0,0,0);

	t->cur = a;

	expect(0, frag_insert(t, 1, untag(b)));

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
	struct frag t[1] = {{0}};

	c = make_tree(3, 0, 0, 0);
	b = make_tree(2, 0, 0, 0);
	a = make_tree(1, 1, 0, b);

	t->cur = a;

	expect(0, frag_insert(t, 3, untag(c)));

	try(frag_flush(t));

	ok(t->cur == b);

	expect_is_root(b);
	expect_has_chld(b, 0, a ^ 3);
	expect_has_chld(b, 1, c);

	expect_is_leaf(a);
	expect_is_leaf(c);
}

void
test_insert_balance_soft(void)
{
	struct frag t[1]={{0}};
	uintptr_t a, b, c;

	a = make_tree(1,  0, 0, 0);
	b = make_tree(2, -1, a, 0);
	c = make_tree(3,  0, 0, 0);

	t->cur = b;

	try(frag_insert(t, 3, untag(c)));

	expect_has_chld(b ^ 2, 1, c);
}

void
test_insert_head(void)
{
	struct frag t[1] = {{0}};
	uintptr_t a, b;

	a = make_tree(4, 0, 0, 0);
	b = make_tree(6, 0, 0, 0);

	t->cur = a;

	expect(0, frag_insert(t, 0, untag(b)));

	expect(6, get_off(a, 0));
	expect(0, get_off(a, 1));

	expect_is_leaf(b);
	expect_is_root(a);
	expect_has_chld(a^2, 0, b);

	kill_tree(a);
}

void
test_insert_tail(void)
{
	struct frag t[1] = {{0}};
	uintptr_t a, b;

	a = make_tree(4, 0, 0, 0);
	b = make_tree(6, 0, 0, 0);

	t->cur = a;

	expect(0, frag_insert(t, 4, untag(b)));

	expect(0, get_off(a, 0));
	expect(6, get_off(a, 1));

	expect_is_leaf(b);
	expect_is_root(a);
	expect_has_chld(a^2, 1, b);

	kill_tree(a);
}

void
test_rebalance_expand_single(void)
{
	uintptr_t a, b, c;

	c = make_tree(3,0,0,0);
	b = make_tree(2,0,0,0);
	a = make_tree(1,1,0,b);

	set_link(b, 1, c), set_link(c, 2, b);

	try(rebalance(untag(c), 1));

	expect_is_root(b);
	expect_has_chld(b, 1, c);
	expect_has_chld(b, 0, a ^ 3);
	
	expect(3, get_off(b, 1));
}

void
test_rebalance_expand_two(void)
{
	struct frag_node *g;
	uintptr_t a, b;

	a = make_tree(1,0,0,0);
	b = make_tree(2,0,0,0);

	set_link(b, 0, a);
	set_link(a, 2, b);

	g = untag(a);
	try(rebalance(g, 1));

	expect(2, tag_of(g->link[2]));
	g = untag(b);
	expect(1, g->off[0]);

	kill_tree(b);
}

void
test_rotate_left(void)
{
	struct frag_node f[1]={{.len=10}}, g[1]={{.len=20}}, h[1]={{.len=30}};
	uintptr_t a, b;

	a = make_tree(1, 1, tag0(f), b = make_tree(2, 0, tag0(g), tag0(h)));

	ok(rotate(a, 0) == b);

	expect_has_chld(a, 0, tag0(f));
	expect_has_chld(a, 1, tag0(g));

	expect_has_chld(b, 0, a);
	expect_has_chld(b, 1, tag0(h));

	expect_is_root(b);

	expect(10, get_off(a, 0));
	expect(20, get_off(a, 1));
	expect(31, get_off(b, 0));
	expect(30, get_off(b, 1));
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
	struct frag_node f[1]={{.len=10}}, g[1]={{.len=20}}, h[1]={{.len=30}};
	uintptr_t a, b;

	a = make_tree(1, -1, b = make_tree(2, 0, tag0(f), tag0(g)), tag0(h));

	ok(rotate(a, 1) == b);

	expect_has_chld(a, 0, tag0(g));
	expect_has_chld(a, 1, tag0(h));

	expect_has_chld(b, 0, tag0(f));
	expect_has_chld(b, 1, a);

	expect_is_root(b);

	expect(20, get_off(a, 0));
	expect(30, get_off(a, 1));
	expect(10, get_off(b, 0));
	expect(51, get_off(b, 1));
}

void
test_rotate2_left(void)
{
	uintptr_t a, b, c;
	uintptr_t e, f, g, h;

	a = make_tree(1, 1,
	              e = make_tree(5,0,0,0),
	              b = make_tree(2, -1,
	                            c = make_tree(3, 0,
	                                        f = make_tree(10,0,0,0),
	                                        g = make_tree(15,0,0,0)),
	                            h = make_tree(20,0,0,0)));

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

	expect(5, get_off(a, 0));
	expect(10, get_off(a, 1));

	expect(15, get_off(b, 0));
	expect(20, get_off(b, 1));

	expect(16, get_off(c, 0));
	expect(37, get_off(c, 1));

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
	                          e = make_tree(5,0,0,0),
	                          c = make_tree(3, 0,
	                                    f = make_tree(10,0,0,0),
	                                    g = make_tree(15,0,0,0))),
	            h = make_tree(20,0,0,0));

	ok(rotate2(b, 1) == c);

	expect_has_chld(a, 0, e);
	expect_has_chld(a, 1, f);

	expect_has_chld(b, 0, g);
	expect_has_chld(b, 1, h);

	expect_has_chld(c, 0, a);
	expect_has_chld(c, 1, b);

	expect_is_root(c);

	expect(5, get_off(a, 0));
	expect(10, get_off(a, 1));

	expect(15, get_off(b, 0));
	expect(20, get_off(b, 1));

	expect(17, get_off(c, 0));
	expect(36, get_off(c, 1));

	kill_tree(b);
}

void
test_stab_absent(void)
{
	struct frag_node root[1]={{.len = 10}};
	struct frag fg[1] = {{0}};

	try(frag_insert(fg, 0, root));
	ok(!frag_stab(fg, 11));
}

void
test_stab_empty(void)
{
	struct frag fg[1] = {{0}};

	ok(!frag_stab(fg, 5));
}

void
test_stab_nearest(void)
{
	struct frag_node root[1] = {{.len = 4}};
	struct frag fg[1] = {{0}};

	expect(0, frag_insert(fg, 0, root));

	ok(!frag_stab(fg, 4));
	ok(untag(fg->cur) == root);
	ok(!frag_stab(fg, 5));
	ok(untag(fg->cur) == root);
}

void
test_stab_root(void)
{
	struct frag_node root[1]={{.len = 10}};
	struct frag fg[1] = {{0}};

	try(frag_insert(fg, 0, root));
	ok(frag_stab(fg, 0));
	ok(frag_stab(fg, 4));
	ok(frag_stab(fg, 0) == root);
	ok(frag_stab(fg, 4) == root);
}

void
test_swap_succ(void)
{
	struct frag_node f[1]={{.len=10}}, g[1]={{.len=20}};
	uintptr_t a, b;

	b = make_tree(2,-1, 0, tag0(g));
	a = make_tree(1, 0, tag0(f), b);

	try(swap(a, 1));

	ok(get_chld(a, 0) == 0);
	ok(get_chld(a, 1) == tag0(g));
	ok(get_prnt(a) == b);

	ok(get_chld(b, 0) == tag0(f));
	ok(get_chld(b, 1) == a);
	ok(get_prnt(b) == 0);

	expect(0, get_off(a, 0));
	expect(20, get_off(a, 1));

	expect(10, get_off(b, 0));
	expect(21, get_off(b, 1));
}
