#include <unit.h>
#include <util.h>

#include <frag.c>

/*
 * TODO:
 * - create a function to create fragment graphs on the fly
 * - test_find_nearest and test_insert_tail are failing because
 *   struct frag maintains state. to fix, we must allow frag_find
 *   to walk up the graph in search of pieces
 */

static void test_delete_absent(void);
static void test_delete_empty(void);
static void test_delete_root(void);

static void test_find_nearest(void);

static void test_finger(void);
static void test_finger_flush(void);

static void test_insert_bottom(void);
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
	{.msg = "should flush stab finger",
	 .fun = unit_list(test_finger_flush),},

	{.msg = "should delete root pieces",
	 .fun = unit_list(test_delete_root),},
	{.msg = "should do nothing when deleting absent pieces",
	 .fun = unit_list(test_delete_absent),},
	{.msg = "should insert pieces at the bottom of the graph",
	 .fun = unit_list(test_insert_bottom),},
};

void
test_delete_absent(void)
{
	struct frag_node one[1]={{.len=5}};
	struct frag fg[1] = {{0}};

	ok(!frag_insert(fg, one));
	try(frag_delete(fg, 9));
	ok(fg->chld);
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
	ok(!fg->chld);
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

	ok(untag(fg->prnt) == one);
	ok(untag(fg->chld) == two);
}

void
test_finger_flush(void)
{
	struct frag_node one[1] = {{.off = 0, .len = 4}};
	struct frag_node two[1] = {{.off = 0, .len = 6}};
	struct frag fg[1] = {{0}};

	try(frag_insert(fg, one));
	try(frag_insert(fg, two));

	try(frag_flush(fg));
}

void
test_insert_bottom(void)
{
	struct frag_node d[4] = {
		{.off = 0, .len = 3,},
		{.off = 0, .len = 2,},
		{.off = 3, .len = 4,},
		{.off = 9, .len = 5,},
	};
	struct frag fg[1] = {{0}};

	__builtin_trap();
 
	ok(!frag_insert(fg, d + 0));
	ok(!frag_insert(fg, d + 1));
	ok(!frag_insert(fg, d + 2));
	ok(!frag_insert(fg, d + 3));

	
}

void
test_insert_empty(void)
{
	struct frag_node node[1] = {{.off = 0, .len = 4,}};
	struct frag fg[1] = {{0}};

	try(frag_insert(fg, node));

	ok(untag(fg->chld) == node);
	expect(0, node->off);
	expect(4, node->len);
	expect(0, node->wid);
	expect(4, node->dsp);
}

void
test_insert_head(void)
{
	struct frag_node one[1] = {{.off = 0, .len = 4}};
	struct frag_node two[1] = {{.off = 0, .len = 6}};
	struct frag fg[1] = {{0}};

	try(frag_insert(fg, one));
	try(frag_insert(fg, two));

	expect(6, one->off);
	expect(4, one->len);
	expect(0, one->wid);
	expect(10, one->dsp);
	expect(0, two->off);
	expect(6, two->len);
	expect(0, two->wid);
	expect(0, two->dsp);

	ok(untag(one->link[0]) == two);
}

void
test_insert_tail(void)
{
	struct frag_node one[1] = {{.off = 0, .len = 4}};
	struct frag_node two[1] = {{.off = 4, .len = 6}};
	struct frag fg[1] = {{0}};

	try(frag_insert(fg, one));
	try(frag_insert(fg, two));

	expect(0, one->off);
	expect(4, one->len);
	expect(6, one->wid);
	expect(10, one->dsp);

	ok(untag(one->link[1]) == two);
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

int
main(int argc, char **argv)
{
	unit_parse_argv(argc, argv);
	return unit_run_tests(tests, arr_len(tests));
}
