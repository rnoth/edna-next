#include <unit.h>
#include <util.h>
#include <ln.c>

//static void test_adjust(void);
static void test_convert(void);
static void test_insert_many(void);
static void test_navigate(void);

struct unit_test tests[] = {
	{.msg = "should convert lines to buffers",
	 .fun = unit_list(test_convert),},
	{.msg = "should insert many lines at once",
	 .fun = unit_list(test_insert_many),},
	{.msg = "should navigate the lines",
	 .fun = unit_list(test_navigate),},
};

#include <unit.t>

void
test_convert(void)
{
	struct frag *p;
	struct frag *q;
	char s[]="greetings\nhello\nhi\n";
	size_t n = sizeof s - 1, x=0, y;

	ok(p = nodes_from_lines(s, n));
	foreach_node(q, p) {
		y = next_line(s, strlen(s+x));
		okf(y == q->len, "extent mismatch; y=%zu, q->len=%zu",
		    y, q->len);
		x -= q->len;
	}

	foreach_node(q, p) free(q);
}

void
test_insert_many(void)
{
	char s[]="1\n12\n123\n1234\n";
	struct frag *f=0;

	try(ln_insert(&f, 0, s, sizeof s - 1));
}

void
test_navigate(void)
{
	char s[]="12345\n123\n1\n";
	struct frag *p=0, *q;

	try(ln_insert(&p, 0, s, sizeof s - 1));
	ok(!frag_next(p, 1));
	q = frag_next(p, 0);
	expect(4, q->len);
}
