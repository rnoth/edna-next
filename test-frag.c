#include <unit.h>
#include <util.h>

#include <frag.c>

static void test_search_empty(void);

struct unit_test tests[] = {
	{.msg = "should fail on searching an empty fragment graphs",
	 .fun = unit_list(test_search_empty),},
};

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
