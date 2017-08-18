#include <string.h>

#include <unit.h>

#include <util.h>
#include <txt.c>

#define text_insert_str(txt, off, str) \
	text_insert(txt, off, str, strlen(str))

static void make_links(struct piece **);

static void test_delete();
static void test_delete2();
static void test_delete3();
static void test_empty();
static void test_insert();
static void test_insert2();
static void test_insert3();
static void test_link();
static void test_merge();
static void test_traverse();

struct unit_test tests[] = {
	{.msg = "should start with an empty piece chain",
	 .fun = unit_list(test_empty),},
	{.msg = "should be able to link pieces",
	 .fun = unit_list(test_link),},
	{.msg = "should be able to traverse piece chains",
	 .fun = unit_list(test_traverse),},
	{.msg = "should be able to insert text within piece",
	 .fun = unit_list(test_insert),},
	{.msg = "should be able to insert text around pieces",
	 .fun = unit_list(test_insert2),},
	{.msg = "should be able to insert text in an empty piece chain",
	 .fun = unit_list(test_insert3),},
	{.msg = "should be able to delete text within pieces",
	 .fun = unit_list(test_delete),},
	{.msg = "should be able to delete across pieces",
	 .fun = unit_list(test_delete2),},
	{.msg = "should be able to delete across many pieces",
	 .fun = unit_list(test_delete3),},
	{.msg = "should merge pieces when appropriate",
	 .fun = unit_list(test_merge),},
};

void
make_links(struct piece **pies)
{
	size_t i;

	for (i=0; pies[i]; ++i) {
		text_link(pies[i], pies[i+1]);
		if (pies[i]->buffer) {
			pies[i]->length = strlen(pies[i]->buffer);
		}
	}
}
#define make_links(...) make_links((struct piece *[]){__VA_ARGS__, 0})

void
test_delete()
{
	struct piece beg[1] = {{0}};
	struct piece pie[1] = {{.buffer="tyypo"}};
	struct piece end[1] = {{0}};
	struct piece *new;
	struct piece *links[2];

	make_links(beg, pie, end);

	links[0] = beg, links[1] = 0;
	text_walk(links, 3);

	expect(0, text_delete(links, 2, 1));

	expect(2, pie->length);
	ok(text_next(pie, beg) != end);
	new = text_next(pie, beg);
	ok(text_next(new, pie) == end);

	ok(links[0] == 0);

	free(new);
}

void
test_delete2()
{
	struct piece beg[1] = {{0}};
	struct piece end[1] = {{0}};
	struct piece one[1] = {{.buffer="abc__"}};
	struct piece two[1] = {{.buffer="__def"}};
	struct piece *links[2];

	make_links(beg, one, two, end);

	links[0] = beg, links[1] = 0;
	text_walk(links, 3);

	expect(0, text_delete(links, 3, 4));

	expect(3, one->length);
	expect(3, two->length);
	ok(*two->buffer == 'd');
}

void
test_delete3()
{
	struct piece beg[1] = {{0}};
	struct piece end[1] = {{0}};
	struct piece one[1] = {{.buffer="hello, "}};
	struct piece two[1] = {{.buffer="(not you)"}};
	struct piece thr[1] = {{.buffer="friends."}};
	struct piece *links[2];

	make_links(beg, one, two, thr, end);

	links[0] = beg, links[1] = 0;
	text_walk(links, 7);

	expect(0, text_delete(links, 0, 9));

	expect(7, one->length);
	expect(8, thr->length);

	ok(*links == two);
	ok(text_next(one, beg) == thr);
	ok(text_next(thr, end) == one);
}

void
test_empty()
{
	struct piece *one;
	struct piece *two;

	ok(one = text_ctor());
	ok(one->link);
	ok(!one->buffer);
	ok(!one->length);
	ok(two = text_next(one, 0));
	ok(two->link == (uintptr_t)one);
	ok(one->link == (uintptr_t)two);
	ok(!two->buffer);
	ok(!two->length);

	try(text_dtor(one));
}

void
test_insert()
{
	struct piece beg[1] = {{0}};
	struct piece hi[1] = {{.buffer="hello!", .length=6}};
	struct piece end[1] = {{0}};
	struct piece *new;
	struct piece *new1;
	struct piece *links[2];

	make_links(beg, hi, end);

	links[0] = beg, links[1] = 0;
	text_walk(links, 5);
	ok(!text_insert_str(links, 5, ", friend"));

	ok(hi->length == 5);

	ok(new = text_next(hi, beg));
	ok(new != hi);
	ok(new != beg);
	ok(new != end);

	new1 = text_next(new, hi);
	ok(new1 != new);
	ok(new1 != end);

	ok(new1->length == 1);
	ok(*new1->buffer == '!');

	links[0] = beg, links[1] = 0;
	text_walk(links, 13);

	ok(links[0] == new1);
	ok(links[1] == new);

	free(new1);
	free(new);
}

void
test_insert2()
{
	struct piece beg[1] = {{0}};
	struct piece pie[1] = {{.buffer="two, ", .length=5}};
	struct piece end[1] = {{0}};
	struct piece *new;
	struct piece *new1;
	struct piece *links[2];

	make_links(beg, pie, end);

	links[0] = beg, links[1] = 0;

	ok(!text_insert_str(links, 0, "one, "));
	ok(new = text_next(beg, 0));
	ok(new != pie);

	ok(text_next(new, beg) == pie);
	ok(pie->length == 5);

	links[0] = beg, links[1] = 0;
	ok(!text_insert_str(links, 10, "three."));
	ok(new1 = text_next(end, 0));
	ok(new1 != pie);
	
	ok(text_next(new1, end) == pie);
	ok(beg->length == 0);

	free(new);
	free(new1);
}

void
test_insert3()
{
	struct piece beg[1] = {{0}};
	struct piece end[1] = {{0}};
	struct piece *new;
	struct piece *links[2];

	make_links(beg, end);

	links[0] = beg, links[1] = 0;

	try(text_insert_str(links, 0, "I'm in"));

	ok(new = text_next(beg, 0));
	ok(new != end);

	ok(text_next(new, beg) == end);
	ok(text_next(end, new) == 0);

	ok(beg->length == 0);
	ok(end->length == 0);
}

void
test_link()
{
	struct piece one[1] = {0};
	struct piece two[1] = {0};
	struct piece thr[1] = {0};

	try(text_link(one, thr));

	ok(one == text_next(thr, 0));
	ok(thr == text_next(one, 0));

	try(text_unlink(one, thr));
	ok(0 == text_next(one, 0));
	ok(0 == text_next(thr, 0));

	try(text_link(one, two));
	try(text_link(two, thr));

	ok(one == text_next(two, thr));
	ok(0 == text_next(thr, two));
	ok(two == text_next(thr, 0));
	ok(two == text_next(one, 0));

	try(text_unlink(one, two));
	try(text_unlink(two, thr));

	try(text_link(one, thr));
	try(text_relink(one, two, thr));

	ok(one == text_next(two, thr));
	ok(0 == text_next(thr, two));
	ok(two == text_next(thr, 0));
	ok(two == text_next(one, 0));
}

void
test_merge(void)
{
	struct piece beg[1]={{0}};
	struct piece pie[1]={{.buffer="~~><~~"}};
	struct piece end[1]={{0}};
	struct piece *ctx[2];

	make_links(beg, pie, end);

	ctx[0] = beg, ctx[1] = 0;
	expect(0, text_insert_str(ctx, 3, "!"));

	ctx[0] = beg, ctx[1] = 0;
	expect(0, text_delete(ctx, 3, 1));
	ok(text_next(pie, beg) == end);
	ok(text_next(pie, end) == beg);
	ok(pie->length = 6);
}

void
test_traverse()
{
	struct piece beg[1] = {{0}};
	struct piece foo[1] = {{.buffer="foo", .length=3}};
	struct piece bar[1] = {{.buffer="bar", .length=3}};
	struct piece baz[1] = {{.buffer="baz", .length=3}};
	struct piece end[1] = {{0}};
	struct piece *links[2];

	text_link(beg, foo);
	text_link(foo, bar);
	text_link(bar, baz);
	text_link(baz, end);

	links[0] = beg, links[1] = 0;
	try(text_walk(links, 0));
	ok(links[0] == foo);
	ok(links[1] == beg);

	links[0] = beg, links[1] = 0;
	try(text_walk(links, 2));
	ok(links[0] == foo);
	ok(links[1] == beg);

	links[0] = beg, links[1] = 0;
	try(text_walk(links, 3));
	ok(links[0] == bar);
	ok(links[1] == foo);

	links[0] = beg, links[1] = 0;
	try(text_walk(links, 5));
	ok(links[0] == bar);
	ok(links[1] == foo);

	links[0] = beg, links[1] = 0;
	try(text_walk(links, 6));
	ok(links[0] == baz);
	ok(links[1] == bar);

	links[0] = beg, links[1] = 0;
	try(text_walk(links, 10));
	ok(links[0] == end);
	ok(links[1] == baz);
}

int
main(int argc, char **argv)
{
	unit_parse_argv(argv);
	return unit_run_tests(tests, arr_len(tests));
}
