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
	struct frag *Q;
	struct frag *q;
	char s[]="one\ntwo\nthree\n";
	size_t n = sizeof s - 1, f=n;
	char *t=s+f;

	ok(Q = nodes_from_lines(s, n));
	foreach_node(q, Q) {
		f -= q->len, t -= q->len;
		okf(!strcmp(t, s + f),
		    "extent mismatch. s=%s, t=%s", s + f, t);
	}

	foreach_node(q, Q) free(q);
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
	q = frag_next(p, 0);
	expect(4, q->len);
}
