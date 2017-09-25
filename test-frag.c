#include <unit.h>
#include <util.h>

#include <tag.h>
#include <frag.c>

static void test_adjust_branches(void);
static void test_adjust_leaves(void);
static void test_adjust_parent(void);
static void test_adjust_single(void);

static void test_delete_absent(void);
static void test_delete_branch(void);
static void test_delete_empty(void);
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

struct unit_test tests[] = {
	{.msg = "should fail on stabbing an empty graphs",
	 .fun = unit_list(test_stab_empty),},
	{.msg = "should insert a fragment to an empty graph",
	 .fun = unit_list(test_insert_empty),},
	{.msg = "should do nothing when deleting from an empty graph",
	 .fun = unit_list(test_delete_empty),},

	{.msg = "should stay at nearest piece in the graph after stabbing",
	 .fun = unit_list(test_stab_nearest),},

	{.msg = "should stab the root piece",
	 .fun = unit_list(test_stab_root),},
	{.msg = "should not stab pieces not in the graph",
	 .fun = unit_list(test_stab_absent),},

	{.msg = "should prepend a fragment into a graph",
	 .fun = unit_list(test_insert_head),},
	{.msg = "should append a fragment into a graph",
	 .fun = unit_list(test_insert_tail),},
	{.msg = "should update balance information on insert",
	 .fun = unit_list(test_insert_balance_soft),},

	{.msg = "should maintain last query location",
	 .fun = unit_list(test_finger),},

	{.msg = "should do nothing when flushing an empty graph",
	 .fun = unit_list(test_flush_empty),},
	{.msg = "should do nothing when flushing a one-element graph",
	 .fun = unit_list(test_flush_one),},
	{.msg = "should point at the root after flushing a two-element graph",
	 .fun = unit_list(test_flush_two),},
	{.msg = "should do nothing when flushing with the finger at the root",
	 .fun = unit_list(test_flush_idempotent),},
	{.msg = "should reset the displacement after flushing a graph",
	 .fun = unit_list(test_flush_offset),},

	{.msg = "should delete root pieces",
	 .fun = unit_list(test_delete_root),},
	{.msg = "should do nothing when deleting absent pieces",
	 .fun = unit_list(test_delete_absent),},

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

	{.msg = "should rebalance the tree with single rotations on insertion",
	 .fun = unit_list(test_insert_balance_single),},
	{.msg = "should rebalance the tree with double rotations on insertion",
	 .fun = unit_list(test_insert_balance_double),},

	{.msg = "should delete branch nodes",
	 .fun = unit_list(test_delete_branch),},
};

#include <unit.t>

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

	expect(3, tag_of(get_prnt(a)));
	expect(3, tag_of(get_prnt(e)));

	try(kill_tree(c));
}

void
test_adjust_leaves(void)
{
	struct frag_node *g;
	uintptr_t a, b, c;

	b = make_tree(2, 0, a = make_tree(1, 0, 0, 0),
	                    c = make_tree(3, 0, 0, 0));

	try(adjust_balance(b, 3));

	g = untag(a);
	expect(3, tag_of(g->link[2]));
	expect(3, tag_of(g->link[1]));

	g = untag(c);
	expect(3, tag_of(g->link[2]));
	expect(3, tag_of(g->link[0]));

	try(kill_tree(c));
}

void
test_adjust_parent(void)
{
	uintptr_t a, b;

	b = make_tree(2, 0, a = make_tree(1, 0, 0, 0), 0);

	try(adjust_balance(a, 3));
	expect(3, tag_of(get_chld(b, 0)));

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
test_delete_absent(void)
{
	struct frag_node one[1]={{.len=5}};
	struct frag fg[1] = {{0}};

	expect(0, frag_insert(fg, 1, one));
	try(frag_delete(fg, 9));
	ok(fg->cur);
	ok(frag_stab(fg, 2) == one);
}

void
test_delete_branch(void)
{
	struct frag fg[1] = {{0}};
	uintptr_t a, b, c;

	a = make_tree(1, 0,0,0);
	c = make_tree(3, 0,0,0);
	b = make_tree(2, a,c,0);

	fg->cur = b;

	try(frag_delete(fg, 2));
}

void
test_delete_empty(void)
{
	struct frag fg[1] = {{0}};

	try(frag_delete(fg, 0));
}

void
test_delete_leaf(void)
{
	struct frag_node one[1] = {{.len=3}};
	struct frag_node two[1] = {{.len=7}};
	struct frag fg[1] = {{0}};

	expect(0, frag_insert(fg, 0, one));
	expect(0, frag_insert(fg, 3, two));

	try(frag_delete(fg, 6));

	ok(untag(fg->cur) == one);
	ok(fg->cur == (uintptr_t)one);

	ok(!one->link[0]);
	ok(!one->link[1]);
	ok(!one->link[2]);

	ok(!one->off[1]);
	ok(!one->off[0]);
}

void
test_delete_root(void)
{
	struct frag_node root[1] = {{.len = 4}};
	struct frag fg[1] = {{0}};

	expect(0, frag_insert(fg, 0,  root));
	try(frag_delete(fg, 1));
	ok(!frag_stab(fg, 1));
	ok(!fg->cur);
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

	expect(0, tag_of(get_prnt(a)));
	expect(0, tag_of(get_prnt(c)));
}

void
test_increment_rotate(void)
{
	uintptr_t a, b, c;

	a = make_tree(1, -1, b = make_tree(2, -1, c = make_tree(3,0,0,0),0),0);

	try(increment_chld(a, 0));

	ok(!get_chld(a, 0));
	ok(!get_chld(a, 1));
	ok(get_prnt(a) == (b ^ 2));

	ok(!get_chld(c, 0));
	ok(!get_chld(c, 1));
	ok(get_prnt(c) == (b ^ 2));

	ok(get_chld(b, 0) == c);
	ok(get_chld(b, 1) == (a ^ 2));
	ok(!get_prnt(b))
}

void
test_increment_rotate2(void)
{
	uintptr_t a, b, c;

	a = make_tree(1, 1, 0, c = make_tree(3, -1, b = make_tree(2,0,0,0),0));

	try(increment_chld(a, 1));

	ok(get_prnt(a) == b);
	ok(get_prnt(c) == b);
	ok(get_chld(b, 0) == (a ^ 3));
	ok(get_chld(b, 1) == (c ^ 2));
	ok(get_prnt(b) == 0);

}

void
test_insert_empty(void)
{
	struct frag_node node[1] = {{.len = 4,}};
	struct frag fg[1] = {{0}};

	expect(0, frag_insert(fg, 0, node));

	ok(untag(fg->cur) == node);
	expect(0, node->off[1]);
	expect(0, node->off[0]);
	ok(!tag_of(fg->cur));
}

void
test_insert_balance_double(void)
{
	uintptr_t a, b, c;
	struct frag fg[1]={{0}};

	a = make_tree(1, 1, 0, c = make_tree(3,0,0,0));
	b = make_tree(2, 0,0,0);

	fg->cur = a;

	expect(0, frag_insert(fg, 1, untag(b)));

	ok(get_chld(b, 0) == (a ^ 3));
	ok(get_chld(b, 1) == c);
	ok(get_prnt(b) == 0);

	ok(get_chld(a, 0) == 0);
	ok(get_chld(a, 1) == 0);
	ok(get_prnt(a) == b);

	ok(get_chld(c, 0) == 0);
	ok(get_chld(c, 1) == 0);
	ok(get_prnt(c) == b);
}

void
test_insert_balance_single(void)
{
	struct frag_node one[1]={{.len=1}};
	struct frag_node two[1]={{.len=2}};
	struct frag_node thr[1]={{.len=3}};
	struct frag fg[1] = {{0}};

	expect(0, frag_insert(fg, 0, one));
	expect(0, frag_insert(fg, 1, two));
	expect(0, frag_insert(fg, 2, thr));

	try(frag_flush(fg));

	ok(untag(fg->cur) == two);
	ok(fg->cur == (uintptr_t)two);
	ok(two->link[2] == 0);
	ok(two->link[0] == (uintptr_t)one);
	ok(two->link[1] == (uintptr_t)thr);

	ok(one->link[0] == 0);
	ok(one->link[1] == (uintptr_t)two);
	ok(one->link[2] == (uintptr_t)two);

	ok(thr->link[0] == (uintptr_t)two);
	ok(thr->link[1] == 0);
	ok(thr->link[2] == (uintptr_t)two);
}

void
test_insert_balance_soft(void)
{
	struct frag_node a[1]={{.len=1}};
	struct frag_node b[1]={{.len=2}};
	struct frag_node c[1]={{.len=3}};
	struct frag fg[1]={{0}};

	try(frag_insert(fg, 0, b));
	try(frag_insert(fg, 0, a));
	try(frag_insert(fg, 3, c));

	ok(c->link[2] == (uintptr_t)b);
}

void
test_insert_head(void)
{
	struct frag_node one[1] = {{.len = 4}};
	struct frag_node two[1] = {{.len = 6}};
	struct frag fg[1] = {{0}};

	expect(0, frag_insert(fg, 0, one));
	expect(0, frag_insert(fg, 0, two));

	expect(6, one->off[0]);
	expect(0, one->off[1]);

	expect(0, two->off[0]);
	expect(0, two->off[1]);

	ok(!one->link[2]);
	ok(untag(one->link[0]) == two);
	ok(!one->link[1]);

	ok(untag(two->link[2]) == one);
	ok(two->link[1] == (uintptr_t)one + 2);

	expect(2, tag_of(two->link[2]));
}

void
test_insert_tail(void)
{
	struct frag_node one[1] = {{.len = 4}};
	struct frag_node two[1] = {{.len = 6}};
	struct frag fg[1] = {{0}};

	expect(0, frag_insert(fg, 0, one));
	expect(0, frag_insert(fg, 4, two));

	expect(6, one->off[1]);
	expect(0, one->off[0]);

	expect(0, two->off[1]);
	expect(0, two->off[0]);

	ok(!one->link[2]);
	ok(untag(one->link[1]) == two);
	ok(!one->link[0]);

	ok(untag(two->link[2]) == one);
	ok(two->link[0] == (uintptr_t)one + 3);

	expect(3, tag_of(two->link[2]));
}

void
test_rotate_left(void)
{
	struct frag_node f[1]={{.len=10}}, g[1]={{.len=20}}, h[1]={{.len=30}};
	uintptr_t a, b;

	a = make_tree(1, 1, tag0(f), b = make_tree(2, 0, tag0(g), tag0(h)));

	ok(rotate(a, 0) == b);

	ok(get_chld(a, 0) == tag0(f));
	ok(get_chld(a, 1) == tag0(g));

	ok(get_chld(b, 0) == a);
	ok(get_chld(b, 1) == tag0(h));
	ok(get_prnt(b) == 0);

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

	ok(get_prnt(a) == b);
	ok(get_prnt(b) == 0);
	ok(get_chld(a, 0) == 0);
	ok(get_chld(a, 1) == 0);
	ok(get_chld(b, 0) == a);
	ok(get_chld(b, 1) == 0);
}

void
test_rotate_right(void)
{
	struct frag_node f[1]={{.len=10}}, g[1]={{.len=20}}, h[1]={{.len=30}};
	uintptr_t a, b;

	a = make_tree(1, -1, b = make_tree(2, 0, tag0(f), tag0(g)), tag0(h));

	ok(rotate(a, 1) == b);

	ok(get_chld(a, 0) == tag0(g));
	ok(get_chld(a, 1) == tag0(h));

	ok(get_chld(b, 0) == tag0(f));
	ok(get_chld(b, 1) == a);
	ok(get_prnt(b) == 0);

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

	ok(get_prnt(a) == b);
	ok(get_prnt(c) == b);
	ok(get_prnt(b) == 0);
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
