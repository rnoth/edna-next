#include <unit.h>
#include <ext.c>

#include <util.h>

static void test_add_two(void);
static void test_add_simple(void);
static void test_insert(void);
static void test_iter(void);
static void test_iter_edge(void);
static void test_query(void);

static struct unit_test tests[] = {
	{.msg = "should be able append a single extent",
	 .fun = unit_list(test_add_simple),},
	{.msg = "should be able to append two extents",
	 .fun = unit_list(test_add_two),},
	{.msg = "should be able to query extents",
	 .fun = unit_list(test_query),},
	{.msg = "should be able to insert extents",
	 .fun = unit_list(test_insert),},
	{.msg = "should be able to iterate over an extent tree",
	 .fun = unit_list(test_iter),},
	{.msg = "should handle edge cases with iterating",
	 .fun = unit_list(test_iter_edge),}
};

void
test_add_two(void)
{
	struct ext_node a[1]={{.ext=3}};
	struct ext_node b[1]={{.ext=1}};
	struct ext_node c[1]={{.ext=7}};
	struct ext ext[1]={0};

	ext_append(ext, a);
	ext_append(ext, b);
	expect(4, ext->len);

	try(ext_append(ext, c));
	expect(11, ext->len);
	ok(ext->root = tag_node(b));

	ok(b->chld[0] == tag_leaf(a));
	ok(b->chld[1] == tag_node(c));

	ok(c->chld[0] == tag_leaf(b));
	ok(c->chld[1] == tag_leaf(c));
}

void
test_add_simple(void)
{
	struct ext_node a[1]={{.ext=1}};
	struct ext_node b[1]={{.ext=2}};
	struct ext ext[1]={0};

	try(ext_append(ext, a));
	ok(ext->root == tag_leaf(a));
	expect(1, ext->len);

	try(ext_append(ext, b));
	ok(ext->root == tag_node(b));
	expect(3, ext->len);

	expect(1, b->off);
	ok(b->chld[0] == tag_leaf(a));
	ok(b->chld[1] == tag_leaf(b));

}

void
test_insert(void)
{
	struct ext_node a[1]={{.ext=100}};
	struct ext_node b[1]={{.ext=250}};
	struct ext_node c[1]={{.ext=75}};
	struct ext ext[1]={0};

	try(ext_insert(ext, a, 0));
	ok(ext->root == tag_leaf(a));
	try(ext_insert(ext, b, 0));
	ok(ext->root == tag_node(b));

	ok(b->chld[0] == tag_leaf(b));
	ok(b->chld[1] == tag_leaf(a));

	ok(ext_stab(ext, 99) == b);
	ok(ext_stab(ext, 300) == a);

	try(ext_insert(ext, c, 250));

	ok(ext->root == tag_node(b));
	ok(b->chld[0] == tag_leaf(b));
	ok(b->chld[1] == tag_node(c));

	ok(c->chld[0] == tag_leaf(c));
	ok(c->chld[1] == tag_leaf(a));
}

void
test_iter(void)
{
	struct ext_node a[1]={{.ext=100}};
	struct ext_node b[1]={{.ext=250}};
	struct ext_node c[1]={{.ext=75}};
	struct ext_walker w[1];
	struct ext x[1]={{0}};

	ext_insert(x, a, 0);
	ext_insert(x, b, 0);
	ext_insert(x, c, 250);

	ok(ext_iterate(w, x) == b);
	ok(ext_continue(w) == c);
	ok(ext_continue(w) == a);
	ok(ext_continue(w) == 0);
	ok(ext_continue(w) == 0);
}

void
test_iter_edge(void)
{
	struct ext empty[1]={{0}};
	struct ext smol[1]={{0}};
	struct ext_node leaf[1]={{.ext=56}};
	struct ext_walker w[1];
	
	ok(ext_iterate(w, empty) == 0);
	ok(ext_continue(w) == 0);

	ext_append(smol, leaf);

	ok(ext_iterate(w, smol) == leaf);
	ok(ext_continue(w) == 0);
}

void
test_query(void)
{
	struct ext_node a[1]={{.ext=22}};
	struct ext_node b[1]={{.ext=455}};
	struct ext_node c[1]={{.ext=123}};
	struct ext ext[1]={0};

	try(ext_append(ext, a));
	try(ext_append(ext, b));
	try(ext_append(ext, c));

	ok(ext_stab(ext, 11) == a);
	ok(ext_stab(ext, 470) == b);
	ok(ext_stab(ext, 599) == c);
}

int
main(int argc, char **argv)
{
	unit_parse_argv(argv);
	return unit_run_tests(tests, arr_len(tests));
}
