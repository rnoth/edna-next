#include <unit.h>
#include <util.h>

#include <frag.c>

static void test_insert_empty(void);
static void test_search_empty(void);

struct unit_test tests[] = {
	{.msg = "should fail on searching an empty fragment graphs",
	 .fun = unit_list(test_search_empty),},
	{.msg = "should insert a fragment to an empty fragment graph",
	 .fun = unit_list(test_insert_empty),},
};

void
test_insert_empty(void)
{
	struct frag fg[1] = {{0}};
	struct frag_node node[1] = {{.off = 0, .len = 4,}};

	try(frag_insert(fg, node));

	ok(untag(fg->chld) == node);
	expect(0, node->off);
	expect(4, node->len);
	expect(4, node->wid);
	expect(4, node->dsp);
}

void
test_search_empty(void)
{
	struct frag fg[1] = {{0}};

	ok(!frag_search(fg, 5));
}

int
main(int argc, char **argv)
{
	unit_parse_argv(argv);
	return unit_run_tests(tests, arr_len(tests));
}
