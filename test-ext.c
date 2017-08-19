#include <unit.h>
#include <ext.c>

#include <util.h>

static void test_add_two(void);
static void test_add_simple(void);
static void test_insert(void);
static void test_iter(void);
static void test_iter_edge(void);
static void test_remove(void);
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
	 .fun = unit_list(test_iter_edge),},
	{.msg = "should be able to remove extents",
	 .fun = unit_list(test_remove),},
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

	try(ext_insert(ext, 0, a));
	ok(ext->root == tag_leaf(a));
	try(ext_insert(ext, 0, b));
	ok(ext->root == tag_node(b));

	ok(b->chld[0] == tag_leaf(b));
	ok(b->chld[1] == tag_leaf(a));

	ok(ext_stab(ext, 99) == b);
	ok(ext_stab(ext, 300) == a);

	try(ext_insert(ext, 250, c));

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

	ext_insert(x, 0, a);
	ext_insert(x, 0, b);
	ext_insert(x, 250, c);

	
	try(ext_iterate(w, x));
	ok(ext_continue(w) == b);
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
	
	try(ext_iterate(w, empty));
	ok(ext_continue(w) == 0);
	ok(ext_continue(w) == 0);

	ext_append(smol, leaf);

	try(ext_iterate(w, smol));
	ok(ext_continue(w) == leaf);
	ok(ext_continue(w) == 0);
}

void
test_remove(void)
{
	struct ext_node a[1]={{.ext=5}};
	struct ext_node b[1]={{.ext=10}};
	struct ext_node c[1]={{.ext=20}};
	struct ext ext[1]={{0}};

	ok(!ext_remove(ext, 5, 1));

	ext_append(ext, a);
	ok(ext_remove(ext, 0, 5) == a);
	ok(!ext->root);

	ext_append(ext, a);
	ext_append(ext, b);
	ok(ext_remove(ext, 0, 5) == a);
	ok(ext->root == tag_leaf(b));
	ok(!b->chld[0] && !b->chld[1]);
	ok(ext_stab(ext, 5) == b);
	ok(!ext_stab(ext, 14));

	ext_insert(ext, 0, a);
	ext_append(ext, c);

	ok(ext_remove(ext, 0, 5) == a);
	ok(ext->root == tag_node(c));
	expect(10, c->off);
	ok(c->chld[0] == tag_leaf(b));
	ok(c->chld[1] == tag_leaf(c));

	ok(ext_stab(ext, 5) == b);
	ok(ext_stab(ext, 15) == c);

	ok(ext_remove(ext, 0, 10) == b);
	ok(ext->root = tag_leaf(c));
	ok(ext_stab(ext, 15) == c);
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
