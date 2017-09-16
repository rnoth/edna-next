#include <unit.h>
#include <util.h>

#include <frag.c>

static void test_balance(void);

static void test_delete_absent(void);
static void test_delete_empty(void);
static void test_delete_root(void);

static void test_find_nearest(void);

static void test_finger(void);
static void test_flush_empty();
static void test_flush_idempotent();
static void test_flush_one();
static void test_flush_two();
static void test_flush_offset();

static void test_insert_empty(void);
static void test_insert_head(void);
static void test_insert_tail(void);

static void test_stab_absent(void);
static void test_stab_empty(void);
static void test_stab_root(void);

struct unit_test tests[] = {
	{.msg = "should fail on stab an empty graphs",
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
	{.msg = "should rotate the tree when unbalanced",
	 .fun = unit_list(test_balance),},
};

#include <unit.t>

void
test_balance(void)
{
	struct frag_node one[1]={{.off=0, .len=1}};
	struct frag_node two[1]={{.off=1, .len=2}};
	struct frag_node thr[1]={{.off=3, .len=3}};
	struct frag fg[1] = {{0}};

	expect(0, frag_insert(fg, one));
	expect(0, frag_insert(fg, two));
	expect(0, frag_insert(fg, thr));

	try(frag_flush(fg));

	ok(fg->cur == (uintptr_t)two);
	ok(two->link[up] == 0);
	ok(two->link[left] == (uintptr_t)one);
	ok(two->link[right] == (uintptr_t)thr);

	ok(one->link[left] == 0);
	ok(one->link[right] == (uintptr_t)two);
	ok(one->link[up] == (uintptr_t)two);

	ok(thr->link[left] == (uintptr_t)two);
	ok(thr->link[right] == 0);
	ok(thr->link[up] == (uintptr_t)two);
}

void
test_delete_absent(void)
{
	struct frag_node one[1]={{.len=5}};
	struct frag fg[1] = {{0}};

	ok(!frag_insert(fg, one));
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
test_delete_root(void)
{
	struct frag_node root[1] = {{.len = 4}};
	struct frag fg[1] = {{0}};

	ok(!frag_insert(fg, root));
	try(frag_delete(fg, 1));
	ok(!frag_stab(fg, 1));
	ok(!fg->cur);
}

void
test_find_nearest(void)
{
	struct frag_node root[1] = {{.len = 4}};
	struct frag fg[1] = {{0}};

	ok(!frag_insert(fg, root));
	ok(frag_find(fg, 4) == root);
	ok(frag_find(fg, 5) == root);
}

void
test_finger(void)
{
	struct frag_node one[1] = {{.off = 0, .len = 4}};
	struct frag_node two[1] = {{.off = 0, .len = 6}};
	struct frag fg[1] = {{0}};

	try(frag_insert(fg, one));
	try(frag_insert(fg, two));

	ok(untag(fg->cur) == two);
	ok(untag(two->link[up]) == one);
}

void
test_flush_empty(void)
{
	struct frag fg[1] = {{0}};

	try(frag_flush(fg));

	ok(!fg->cur);
	ok(!fg->dsp);
}

void
test_flush_idempotent(void)
{
	struct frag_node one[1] = {{.off = 0, .len = 10}};
	struct frag_node two[1] = {{.off = 0, .len = 5}};
	struct frag fg[1] = {{0}};

	try(frag_insert(fg, one));
	try(frag_insert(fg, two));

	try(frag_flush(fg));

	ok(untag(fg->cur) == one);
	ok(!fg->dsp);

	try(frag_flush(fg));

	ok(untag(fg->cur) == one);
	ok(!fg->dsp);
}

void
test_flush_one(void)
{
	struct frag_node one[1] = {{.off = 0, .len = 2}};
	struct frag fg[1] = {{0}};

	try(frag_insert(fg, one));

	try(frag_flush(fg));

	ok(untag(fg->cur) == one);
	ok(!fg->dsp);
}

void
test_flush_two(void)
{
	struct frag_node one[1] = {{.off = 0, .len = 4}};
	struct frag_node two[1] = {{.off = 0, .len = 6}};
	struct frag fg[1] = {{0}};

	try(frag_insert(fg, one));
	try(frag_insert(fg, two));

	try(frag_flush(fg));

	ok(untag(fg->cur) == one);
	ok(!fg->dsp);
}

void
test_flush_offset(void)
{
	struct frag_node one[1] = {{.off = 0, .len = 4}};
	struct frag_node two[1] = {{.off = 4, .len = 6}};
	struct frag fg[1] = {{0}};

	try(frag_insert(fg, one));
	try(frag_insert(fg, two));

	try(frag_flush(fg));

	ok(untag(fg->cur) == one);
	ok(!fg->dsp);
}

void
test_insert_empty(void)
{
	struct frag_node node[1] = {{.off = 0, .len = 4,}};
	struct frag fg[1] = {{0}};

	try(frag_insert(fg, node));

	ok(untag(fg->cur) == node);
	expect(0, node->off);
	expect(4, node->len);
	expect(0, node->wid);
	expect(4, node->dsp);
	ok(!bal(fg->cur));
}

void
test_insert_head(void)
{
	struct frag_node one[1] = {{.off = 0, .len = 4}};
	struct frag_node two[1] = {{.off = 0, .len = 6}};
	struct frag fg[1] = {{0}};

	expect(0, frag_insert(fg, one));
	expect(0, frag_insert(fg, two));

	expect(6, one->off);
	expect(4, one->len);
	expect(0, one->wid);
	expect(10, one->dsp);
	expect(0, two->off);
	expect(6, two->len);
	expect(0, two->wid);
	expect(0, two->dsp);

	ok(!one->link[up]);
	ok(untag(one->link[left]) == two);
	ok(!one->link[right]);

	ok(untag(two->link[up]) == one);
	ok(!two->link[left]);

	expect(-1, bal(two->link[up]));
}

void
test_insert_tail(void)
{
	struct frag_node one[1] = {{.off = 0, .len = 4}};
	struct frag_node two[1] = {{.off = 4, .len = 6}};
	struct frag fg[1] = {{0}};

	expect(0, frag_insert(fg, one));
	expect(0, frag_insert(fg, two));

	expect(0, one->off);
	expect(4, one->len);
	expect(6, one->wid);
	expect(4, one->dsp);

	expect(0, two->off);
	expect(6, two->len);
	expect(0, two->wid);
	expect(0, two->dsp);

	ok(!one->link[up]);
	ok(untag(one->link[right]) == two);
	ok(!one->link[left]);

	ok(untag(two->link[up]) == one);
	ok(!two->link[right]);

	expect(1, bal(two->link[up]));
}

void
test_stab_absent(void)
{
	struct frag_node root[1]={{.off = 0, .len = 10}};
	struct frag fg[1] = {{0}};

	try(frag_insert(fg, root));
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
	struct frag_node root[1]={{.off = 0, .len = 10}};
	struct frag fg[1] = {{0}};

	try(frag_insert(fg, root));
	ok(frag_stab(fg, 0));
	ok(frag_stab(fg, 4));
	ok(frag_stab(fg, 0) == root);
	ok(frag_stab(fg, 4) == root);
}
