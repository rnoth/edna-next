#include <unit.h>
#include <util.h>

#include <frag.c>

static void test_delete_absent(void);
static void test_delete_empty(void);
static void test_delete_root(void);

static void test_insert_bottom(void);
static void test_insert_empty(void);
static void test_insert_head(void);
static void test_insert_tail(void);

static void test_search_absent(void);
static void test_search_empty(void);
static void test_search_root(void);

struct unit_test tests[] = {
	{.msg = "should fail on searching an empty graphs",
	 .fun = unit_list(test_search_empty),},
	{.msg = "should insert a fragment to an empty graph",
	 .fun = unit_list(test_insert_empty),},
	{.msg = "should do nothing when deleting from an empty graph",
	 .fun = unit_list(test_delete_empty),},
	{.msg = "should prepend a fragment into a graph",
	 .fun = unit_list(test_insert_head),},
	{.msg = "should append a fragment into a graph",
	 .fun = unit_list(test_insert_tail),},
	{.msg = "should find the root piece",
	 .fun = unit_list(test_search_root),},
	{.msg = "should not find pieces not in the graph",
	 .fun = unit_list(test_search_absent),},
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
	ok(frag_search(fg, 2) == one);
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
	ok(!frag_search(fg, 1));
	ok(!fg-> chld);
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

	//ok(untag(fg->prnt) == one);
	ok(untag(fg->chld) == one);
	expect(6, one->off);
	expect(4, one->len);
	expect(0, one->wid);
	expect(10, one->dsp);

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

	//ok(untag(fg->prnt) == one);
	ok(untag(fg->chld) == one);
	expect(0, one->off);
	expect(4, one->len);
	expect(6, one->wid);
	expect(10, one->dsp);

	ok(untag(one->link[1]) == two);
}

void
test_search_absent(void)
{
	struct frag_node root[1]={{.off = 0, .len = 10}};
	struct frag fg[1] = {{0}};

	try(frag_insert(fg, root));
	ok(frag_search(fg, 11) == 0x0);
}

void
test_search_empty(void)
{
	struct frag fg[1] = {{0}};

	ok(!frag_search(fg, 5));
}

void
test_search_root(void)
{
	struct frag_node root[1]={{.off = 0, .len = 10}};
	struct frag fg[1] = {{0}};

	try(frag_insert(fg, root));
	ok(frag_search(fg, 0) == root);
	ok(frag_search(fg, 4) == root);
}

int
main(int argc, char **argv)
{
	unit_parse_argv(argv);
	return unit_run_tests(tests, arr_len(tests));
}
