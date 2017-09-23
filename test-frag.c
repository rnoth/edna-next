#include <unit.h>
#include <util.h>

#include <tag.h>
#include <frag.c>

/* static void test_adjust_branches(void); */
static void test_adjust_leaves(void);
static void test_adjust_single(void);

static void test_delete_absent(void);
static void test_delete_empty(void);
static void test_delete_leaf(void);
static void test_delete_root(void);

static void test_find_nearest(void);

static void test_finger(void);
static void test_flush_empty();
static void test_flush_idempotent();
static void test_flush_one();
static void test_flush_two();
static void test_flush_offset();

static void test_insert_balance(void);
static void test_insert_empty(void);
static void test_insert_head(void);
static void test_insert_tail(void);

static void test_rotate_left(void);
static void test_rotate_right(void);

static void test_rotate2_left(void);
static void test_rotate2_right(void);

static void test_stab_absent(void);
static void test_stab_empty(void);
static void test_stab_root(void);

struct unit_test tests[] = {
	{.msg = "should fail on stabbing an empty graphs",
	 .fun = unit_list(test_stab_empty),},
	{.msg = "should insert a fragment to an empty graph",
	 .fun = unit_list(test_insert_empty),},
	{.msg = "should do nothing when deleting from an empty graph",
	 .fun = unit_list(test_delete_empty),},

	{.msg = "should find nearest piece in the graph",
	 .fun = unit_list(test_find_nearest),},

	{.msg = "should stab the root piece",
	 .fun = unit_list(test_stab_root),},
	{.msg = "should not stab pieces not in the graph",
	 .fun = unit_list(test_stab_absent),},

	{.msg = "should prepend a fragment into a graph",
	 .fun = unit_list(test_insert_head),},
	{.msg = "should append a fragment into a graph",
	 .fun = unit_list(test_insert_tail),},

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
	/* {.msg = "should adjust the balance of nodes with branch children", */
	/*  .fun = unit_list(test_adjust_branches),}, */

	{.msg = "should rotate to the left",
	 .fun = unit_list(test_rotate_left),},
	{.msg = "should rotate to the right",
	 .fun = unit_list(test_rotate_right),},

	{.msg = "should double-rotate to the left",
	 .fun = unit_list(test_rotate2_left),},
	{.msg = "should double-rotate to the right",
	 .fun = unit_list(test_rotate2_right),},

	{.msg = "should rebalance the tree",
	 .fun = unit_list(test_insert_balance),},
};

#include <unit.t>

void
test_adjust_branches(void)
{
	struct frag_node a[1]={{.len=1}};
	struct frag_node b[1]={{.len=2}};
	struct frag_node c[1]={{.len=3}};
	struct frag_node d[1]={{.len=4}};
	struct frag_node e[1]={{.len=5}};
	struct frag fg[1] = {{0}};

	try(frag_insert(fg, 0, c));

	try(frag_insert(fg, 0, a));
	try(frag_insert(fg, 7, e));

	try(frag_insert(fg, 1, b));
	try(frag_insert(fg, 6, d));

	try(frag_flush(fg));
	try(adjust_balance(fg->cur, 3));

	expect(3, tag_of(a->link[2]));
	ok(untag(a->link[1]) == b);

	expect(3, tag_of(e->link[2]));
	ok(untag(e->link[1]) == d);

}

void
test_adjust_leaves(void)
{
	struct frag_node a[1]={{.len=1}};
	struct frag_node b[1]={{.len=2}};
	struct frag_node c[1]={{.len=3}};
	struct frag fg[1] = {{0}};

	try(frag_insert(fg, 0, b));
	try(frag_insert(fg, 0, a));
	try(frag_insert(fg, 3, c));

	try(frag_flush(fg));
	try(adjust_balance(fg->cur, 0));

	expect(0, tag_of(b->link[2]));
	expect(0, tag_of(b->link[0]));

	expect(0, tag_of(c->link[2]));
	expect(0, tag_of(c->link[1]));

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
test_find_nearest(void)
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
test_insert_balance(void)
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
	struct frag_node a[1];
	struct frag_node b[1];
	struct frag_node f[1];
	struct frag_node g[1];
	struct frag_node h[1];

	*a = (struct frag_node){
		.off = {10, 11},
		.len=5,
		.link={tag0(f), tag0(b), 0}};
	*b = (struct frag_node){
		.off = {15, 4},
		.len=7,
		.link={tag0(g), tag0(h), tag0(a),},
	};

	ok(rotate(tag0(a), 0) == tag0(b));

	ok(a->link[1] == tag0(g));
	ok(a->link[2] == tag0(b));

	ok(b->link[0] == tag0(a));
	ok(b->link[2] == 0);

	expect(15, a->off[1]);
	expect(25, b->off[0]);
}

void
test_rotate_right(void)
{
	struct frag_node a[1];
	struct frag_node b[1];
	struct frag_node f[1];
	struct frag_node g[1];
	struct frag_node h[1];

	*a = (struct frag_node){
		.off={19, 11},
		.len=5,
		.link={tag0(b), tag0(h), 0}};
	*b = (struct frag_node){
		.off={15, 7},
		.len=7,
		.link={tag0(f), tag0(g), tag0(a),},
	};

	ok(rotate(tag0(a), 1) == tag0(b));

	ok(a->link[0] == tag0(g));
	ok(a->link[2] == tag0(b));

	ok(b->link[1] == tag0(a));
	ok(b->link[2] == 0);

	expect(7, a->off[0]);
	expect(18, b->off[1]);
}

void
test_rotate2_left(void)
{
	struct frag_node a[1];
	struct frag_node b[1];
	struct frag_node c[1];
	struct frag_node e[1];
	struct frag_node f[1];
	struct frag_node g[1];
	struct frag_node h[1];

	*a = (struct frag_node){
		.off={3, 7},
		.len=0,
		.link={tag0(e), tag0(c), 0, }};

	*c = (struct frag_node){
		.off={3, 4},
		.len=0,
		.link={tag0(b), tag0(h), tag0(a), },
	};

	*b = (struct frag_node){
		.off={1, 2},
		.len=0,
		.link={tag0(f), tag0(g), tag0(a),},
	};

	ok(rotate2(tag0(a), 0) == tag0(b));

	ok(a->link[0] == tag0(e));
	ok(a->link[1] == tag0(f));
	ok(a->link[2] == tag0(b));

	ok(b->link[0] == tag0(a));
	ok(b->link[1] == tag0(c));
	ok(b->link[2] == 0);

	ok(c->link[0] == tag0(g));
	ok(c->link[1] == tag0(h));
	ok(c->link[2] == tag0(b));

	expect(4, c->off[1]);
	expect(3, a->off[0]);

	expect(2, c->off[0]);
	expect(6, b->off[1]);

	expect(1, a->off[1]);
	expect(4, b->off[0]);
}

void
test_rotate2_right(void)
{
	struct frag_node a[1];
	struct frag_node b[1];
	struct frag_node c[1];
	struct frag_node e[1];
	struct frag_node f[1];
	struct frag_node g[1];
	struct frag_node h[1];


	*c = (struct frag_node){
		.off={3, 4},
		.len=0,
		.link={tag0(a), tag0(h), 0},
	};

	*a = (struct frag_node){
		.off={3, 7},
		.len=0,
		.link={tag0(e), tag0(b), tag0(c)}
	};

	*b = (struct frag_node){
		.off={1, 2},
		.len=0,
		.link={tag0(f), tag0(g), tag0(a),},
	};

	ok(rotate2(tag0(c), 1) == tag0(b));

	ok(a->link[0] == tag0(e));
	ok(a->link[1] == tag0(f));

	ok(b->link[0] == tag0(a));
	ok(b->link[1] == tag0(c));

	ok(c->link[0] == tag0(g));
	ok(c->link[1] == tag0(h));

	expect(4, c->off[1]);
	expect(3, a->off[0]);

	expect(2, c->off[0]);
	expect(6, b->off[1]);

	expect(1, a->off[1]);
	expect(4, b->off[0]);
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
