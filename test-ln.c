#include <unit.h>
#include <util.h>
#include <ln.c>

//static void test_adjust(void);
static void test_convert(void);

struct unit_test tests[] = {
	{.msg = "should convert lines to buffers",
	 .fun = unit_list(test_convert),},
	//{.msg = "should adjust lines",
	//.fun = unit_list(test_adjust),},
};

#define UNIT_TESTS tests
#include <unit.t>

void
test_adjust(void)
{
	struct ext lines[1]={{0}};
	struct ext_node *d;
	struct ext_node *e;
	char s[]="---><---";
	char t[]="~";

	ln_insert(lines, 0, s, sizeof s);
	ln_insert(lines, 3, t, sizeof t);

	try(d = ext_stab(lines, 1));
	try(e = ext_stab(lines, 3));
	ok(e == d);
	expect(9, d->ext);
}

void
test_convert(void)
{
	struct ext_node *list;
	struct ext_node *node;
	char s[]="one\ntwo\nthree\n";
	size_t n = sizeof s - 1;
	size_t f=n;
	char *t=s+f;

	ok(list = nodes_from_lines(s, n));
	foreach_node(node, list) {
		f -= node->ext;
		t -= node->ext;
		okf(!strcmp(t, s + f),
		    "extent mismatch. s=%s, t=%s", s + f, t);
	}

	foreach_node(node, list) free(node);
}
