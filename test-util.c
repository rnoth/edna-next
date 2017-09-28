#include <util.h>
#include <unit.h>

static void test_memswp(void);
static void test_memswp_large(void);

struct unit_test tests[] = {
	{.msg = "should swap values",
	 .fun = unit_list(test_memswp),},
	{.msg = "should swap larger values",
	 .fun = unit_list(test_memswp_large),},
};

#include <unit.t>

void
test_memswp(void)
{
	short a=1234, b=5678;

	ok(memswp(&a, &b, sizeof a) == &a);

	ok(a == 5678);
	ok(b == 1234);
}

void
test_memswp_large(void)
{
	char a[] = "abcdefghi";
	char b[] = "jklmnopqr";

	ok(memswp(&a, &b, 10) == a);

	ok(!strcmp(a, "jklmnopqr"));
	ok(!strcmp(b, "abcdefghi"));
}

