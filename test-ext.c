#include <unit.h>
#include <ext.c>

#include <util.h>

static void test_add_two(void);
static void test_add_simple(void);
static void test_insert(void);
static void test_next(void);
static void test_remove(void);
static void test_remove_relink(void);
static void test_query(void);

static struct unit_test tests[] = {
	{.msg = "should be able append a single extent",
	 .fun = unit_list(test_add_simple),},
	{.msg = "should append two extents",
	 .fun = unit_list(test_add_two),},
	{.msg = "should query extents",
	 .fun = unit_list(test_query),},
	{.msg = "should insert extents",
	 .fun = unit_list(test_insert),},
	{.msg = "should remove extents",
	 .fun = unit_list(test_remove),},
	{.msg = "should remove extents consistently",
	 .fun = unit_list(test_remove_relink),},
	{.msg = "should iterate over nodes in an extent tree",
	 .fun = unit_list(test_next),}
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
test_next(void)
{
	struct ext_walker w[1];
	struct ext_node a[1]={{.ext=8}};
	struct ext_node b[1]={{.ext=4}};
	struct ext_node c[1]={{.ext=2}};
	struct ext x[1]={{0}};

	ext_append(x, a);
	ext_append(x, b);
	ext_append(x, c);

	ok(ext_stab(x, 7) == a);
	ok(ext_stab(x, 11) == b);
	ok(ext_stab(x, 13) == c);

	walker_begin(w, x);
	walker_locate(w, 0, 0);
	ok(w->tag == tag_leaf(a));
	try(walker_next(w));
	ok(w->tag == tag_leaf(b));
	try(walker_next(w));
	ok(w->tag == tag_leaf(c));
	try(walker_next(w));
	ok(w->tag == tag_node(c));
	try(walker_next(w));
	ok(w->tag == tag_node(b));
	try(walker_next(w));
	ok(w->tag == tag_node(b));
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
test_remove_relink(void)
{
	struct ext_node a[1]={{.ext=8}};
	struct ext_node b[1]={{.ext=4}};
	struct ext_node c[1]={{.ext=2}};
	struct ext x[1]={{0}};

	ext_append(x, a);
	ext_append(x, b);
	ext_append(x, c);

	ext_remove(x, 0, 8);
	*a = (struct ext_node){0};
	ok(ext_stab(x, 1) == b);
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
