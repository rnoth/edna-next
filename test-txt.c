#include <string.h>

#include <unit.h>

#include <util.h>
#include <txt.c>

static struct piece *make_chain(size_t *);
static void name_links(struct piece ***);

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
	{.msg = "should link pieces",
	 .fun = unit_list(test_link),},
	{.msg = "should traverse piece chains",
	 .fun = unit_list(test_traverse),},
	{.msg = "should insert text within piece",
	 .fun = unit_list(test_insert),},
	{.msg = "should insert text around pieces",
	 .fun = unit_list(test_insert2),},
	{.msg = "should insert text in an empty piece chain",
	 .fun = unit_list(test_insert3),},
	{.msg = "should delete text within pieces",
	 .fun = unit_list(test_delete),},
	{.msg = "should delete across pieces",
	 .fun = unit_list(test_delete2),},
	{.msg = "should delete across many pieces",
	 .fun = unit_list(test_delete3),},
	{.msg = "should merge pieces when appropriate",
	 .fun = unit_list(test_merge),},
};

struct piece *
make_chain(size_t *lens)
{
	struct piece *beg;
	struct piece *prev=0;
	struct piece *pie;
	size_t off=0;
	size_t i;

	ok(beg = prev = calloc(1, sizeof *prev));

	for (i=0; lens[i]; ++i) {
		ok(pie = calloc(1, sizeof *pie));
		text_link(pie, prev);

		pie->length = lens[i];
		pie->offset = off;

		off += pie->length;

		prev = pie;
	}

	pie = calloc(1, sizeof *pie);
	text_link(pie, prev);

	return beg;
}
#define make_chain(...) make_chain((size_t []){__VA_ARGS__, 0})

void
name_links(struct piece ***links)
{
	struct piece *ctx[2];
	size_t i;

	text_start(ctx, *links[0]);
	for (i=1; links[i]; ++i) {
		try(text_step(ctx));
		*links[i] = ctx[0];
	}
}
#define name_links(...) name_links((struct piece **[]){__VA_ARGS__, 0})

void
test_delete()
{
	struct piece *beg, *end, *pie, *pie1;
	struct piece *ctx[2];

	beg = make_chain(5);

	text_start(ctx, beg);
	text_walk(ctx, 3);

	expect(0, text_delete(ctx, 2, 1));

	ok(!ctx[0] && !ctx[1]);

	ok(pie = text_next(beg, 0));
	ok(pie1 = text_next(pie, beg));
	ok(end = text_next(pie1, pie));

	expect(2, pie->length);
	expect(0, pie->offset);
	expect(2, pie1->length);
	expect(3, pie1->offset);

	text_dtor(beg);
}

void
test_delete2()
{
	struct piece *beg, *end, *one, *two;
	struct piece *ctx[2];

	beg = make_chain(6, 6);
	name_links(&beg, &one, &two, &end);

	text_start(ctx, beg);

	expect(0, text_delete(ctx, 4, 4));

	expect(4, one->length);
	expect(0, one->offset);
	expect(4, two->length);
	expect(8, two->offset);
}

void
test_delete3()
{
	struct piece *beg, *end, *one, *two, *thr;
	struct piece *ctx[2];

	beg = make_chain(5, 7, 9);
	name_links(&beg, &one, &two, &thr, &end);

	text_start(ctx, beg);
	expect(0, text_delete(ctx, 5, 7));

	expect(5, one->length);
	expect(9, thr->length);

	ok(ctx[0] == two && !ctx[1]);
	ok(text_next(one, beg) == thr);
	ok(text_next(thr, end) == one);
}

void
test_empty()
{
	struct piece *one, *two;

	ok(one = text_ctor());
	ok(one->link);
	ok(!one->offset);
	ok(!one->length);
	ok(two = text_next(one, 0));
	ok(two->link == (uintptr_t)one);
	ok(one->link == (uintptr_t)two);
	ok(!two->offset);
	ok(!two->length);

	try(text_dtor(one));
}

void
test_insert()
{
	struct piece *beg, *one, *new, *new1, *end;
	struct piece *ctx[2];

	beg = make_chain(4);
	name_links(&beg, &one, &end);

	text_start(ctx, beg);
	ok(!text_insert(ctx, 2, 4, 3));

	expect(2, one->length);

	ok(new = text_next(one, beg));
	ok(new != one);
	ok(new != beg);
	ok(new != end);
	expect(4, new->offset);
	expect(3, new->length);

	new1 = text_next(new, one);
	ok(new1 != new);
	ok(new1 != end);

	expect(2, new1->offset);
	expect(2, new1->length);

	try(text_dtor(beg));
}

void
test_insert2()
{
	struct piece *beg, *end, *pie, *new, *new1;
	struct piece *ctx[2];

	beg = make_chain(10);
	name_links(&beg, &pie, &end);

	text_start(ctx, beg);
	ok(!text_insert(ctx, 0, 10, 5));
	ok(new = text_next(beg, 0));
	ok(new != pie);

	expect(10, new->offset);
	expect(5, new->length);

	ok(text_next(new, beg) == pie);

	text_start(ctx, beg);
	ok(!text_insert(ctx, 15, 15, 5));
	ok(new1 = text_next(end, 0));
	ok(new1 != pie);
	ok(new1 != end);

	ok(new1->offset == 15);
	ok(new1->length == 5);
	
	ok(text_next(new1, end) == pie);

	text_dtor(beg);
}

void
test_insert3()
{
	struct piece *beg, *end, *new;
	struct piece *ctx[2];

	beg = make_chain(0);
	name_links(&beg, &end);

	text_start(ctx, beg);

	try(text_insert(ctx, 0, 0, 3));

	ok(new = text_next(beg, 0));
	ok(new != end);
	ok(text_next(new, beg) == end);
	ok(text_next(end, new) == 0);

	ok(new->offset == 0);
	ok(new->length == 3);

	ok(beg->length == 0);
	ok(end->length == 0);

	text_dtor(beg);
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
	struct piece *beg, *end, *pie;
	struct piece *ctx[2];

	beg = make_chain(6);
	name_links(&beg, &pie, &end);

	text_start(ctx, beg);
	expect(0, text_insert(ctx, 3, 7, 1));

	text_start(ctx, beg);

	expect(0, text_delete(ctx, 3, 1));

	ok(text_next(pie, beg) == end);
	ok(text_next(pie, end) == beg);
	ok(pie->length = 6);

	text_dtor(beg);
	free(ctx[0]);
}

void
test_traverse()
{
	struct piece beg[1] = {{0}};
	struct piece foo[1] = {{.offset=0, .length=3}};
	struct piece bar[1] = {{.offset=3, .length=3}};
	struct piece baz[1] = {{.offset=6, .length=3}};
	struct piece end[1] = {{0}};
	struct piece *ctx[2];

	text_link(beg, foo);
	text_link(foo, bar);
	text_link(bar, baz);
	text_link(baz, end);

	ctx[0] = beg, ctx[1] = 0;
	try(text_walk(ctx, 0));
	ok(ctx[0] == foo);
	ok(ctx[1] == beg);

	ctx[0] = beg, ctx[1] = 0;
	try(text_walk(ctx, 2));
	ok(ctx[0] == foo);
	ok(ctx[1] == beg);

	ctx[0] = beg, ctx[1] = 0;
	try(text_walk(ctx, 3));
	ok(ctx[0] == bar);
	ok(ctx[1] == foo);

	ctx[0] = beg, ctx[1] = 0;
	try(text_walk(ctx, 5));
	ok(ctx[0] == bar);
	ok(ctx[1] == foo);

	ctx[0] = beg, ctx[1] = 0;
	try(text_walk(ctx, 6));
	ok(ctx[0] == baz);
	ok(ctx[1] == bar);

	ctx[0] = beg, ctx[1] = 0;
	try(text_walk(ctx, 10));
	ok(ctx[0] == end);
	ok(ctx[1] == baz);
}

int
main(int argc, char **argv)
{
	unit_parse_argv(argv);
	return unit_run_tests(tests, arr_len(tests));
}
