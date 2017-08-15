#include <unit.h>
#include <ext.c>

#include <util.h>

void
test_double_add(void)
{
	struct ext_node a[1]={{.off=3}};
	struct ext_node b[1]={{.off=1}};
	struct ext_node c[1]={{.off=7}};
	struct ext ext[1]={0};

	ext_append(ext, a);
	ext_append(ext, b);
	try(ext_append(ext, c));

	ok(ext->root == tag_node(c));
	ok(c->chld[0] == tag_node(b));
	ok(c->chld[1] == tag_leaf(c));

	ok(b->chld[0] == tag_node(a));
	ok(b->chld[1] == tag_leaf(b));
}

void
test_simple_add(void)
{
	struct ext_node a[1]={{.ext=1}};
	struct ext_node b[1]={{.ext=2}};
	struct ext ext[1]={0};

	try(ext_append(ext, a));
	ok(ext->root == tag_node(a));

	try(ext_append(ext, b));
	ok(ext->root == tag_node(b));
	ok(b->chld[0] == tag_node(a));
	ok(b->chld[1] == tag_leaf(b));
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

static struct unit_test tests[] = {
	{.msg = "should be able append a single extent",
	 .fun = unit_list(test_simple_add),},
	{.msg = "should be able to append two extents",
	 .fun = unit_list(test_double_add),},
	{.msg = "should be able to query extents",
	 .fun = unit_list(test_query),},
};

int
main(int argc, char **argv)
{
	unit_parse_argv(argv);
	return unit_run_tests(tests, arr_len(tests));
}
