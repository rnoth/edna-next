#include <string.h>

#include <unit.h>

#include <util.h>
#include <txt.c>

static void make_links(struct piece **);

static void test_delete();
static void test_empty();
static void test_insert();
static void test_insert2();
static void test_link();
static void test_traverse();

struct unit_test tests[] = {
	{.msg = "should start with an empty piece list",
	 .fun = unit_list(test_empty),},
	{.msg = "should be able to link pieces",
	 .fun = unit_list(test_link),},
	{.msg = "should be able to traverse piece lists",
	 .fun = unit_list(test_traverse),},
	{.msg = "should be able to insert text within piece",
	 .fun = unit_list(test_insert),},
	{.msg = "should be able to insert text around pieces",
	 .fun = unit_list(test_insert2),},
	{.msg = "should be able to delete text within pieces",
	 .fun = unit_list(test_delete),},
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

	expect(4, pie->length);
	ok(text_next(pie, beg) != end);
	new = text_next(pie, beg);
	ok(text_next(new, pie) == end);
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
	ok(two->link == (link)one);
	ok(one->link == (link)two);
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
	struct piece new[1] = {{.buffer=", friend", .length=8}};
	struct piece *pie;
	struct piece *links[2];

	make_links(beg, hi, end);

	links[0] = beg, links[1] = 0;
	text_walk(links, 5);
	ok(!text_insert(links, new, 5));

	ok(hi->length == 5);

	ok(new == text_next(hi, beg));

	pie = text_next(new, hi);
	ok(pie != end);

	ok(pie->length == 1);
	ok(*pie->buffer == '!');

	links[0] = beg, links[1] = 0;
	ok(text_walk(links, 14));

	ok(links[0] == pie);
	ok(links[1] == new);

	free(pie);
}

void
test_insert2()
{
	struct piece beg[1] = {{0}};
	struct piece one[1] = {{.buffer="two, ", .length=5}};
	struct piece end[1] = {{0}};
	struct piece new[1] = {{.buffer="three.", .length=6}};
	struct piece new1[1] = {{.buffer="one, ", .length=5}};
	struct piece *links[2];

	make_links(beg, one, end);

	links[0] = beg, links[1] = 0;
	text_walk(links, 5);

	ok(!text_insert(links, new, 5));
	ok(text_next(one, beg) == new);
	ok(one->length == 5);

	links[0] = beg, links[1] = 0;
	ok(!text_insert(links, new1, 0));

	ok(text_next(one, new) == new1);
	ok(text_next(beg, 0) == new1);
	ok(beg->length == 0);
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
	ok(links[0] == beg);
	ok(links[1] == 0);

	links[0] = beg, links[1] = 0;
	try(text_walk(links, 2));
	ok(links[0] == foo);
	ok(links[1] == beg);

	links[0] = beg, links[1] = 0;
	try(text_walk(links, 3));
	ok(links[0] == foo);
	ok(links[1] == beg);

	links[0] = beg, links[1] = 0;
	try(text_walk(links, 5));
	ok(links[0] == bar);
	ok(links[1] == foo);

	links[0] = beg, links[1] = 0;
	try(text_walk(links, 6));
	ok(links[0] == bar);
	ok(links[1] == foo);

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
