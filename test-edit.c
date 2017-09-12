#include <stdlib.h>

#include <unit.h>
#include <util.h>
#include <edit.c>

static void test_append();
static void test_create();
static void test_resize();

struct unit_test tests[] = {
	{.msg = "should create edit buffers",
	 .fun = unit_list(test_create),},
	{.msg = "should append text",
	 .fun = unit_list(test_append),},
	{.msg = "should resize edit buffers",
	 .fun = unit_list(test_resize),},
};

void
test_append()
{
	struct map edit[1];
	char buffer[256];

	memset(buffer, 'a', sizeof buffer);

	edit_ctor(edit);
	expect(0, edit_append(edit, buffer, sizeof buffer));
	expect(sizeof buffer, edit->offset);
	ok(!memcmp(edit->map, buffer, sizeof buffer));
}

void
test_create()
{
	struct map edit[1];

	edit_ctor(edit);
	ok(edit->length);
	ok(!edit->offset);
	ok(edit->fd > 0);
	ok(edit->map);
	try(edit->map[0] = 0);
}

void
test_resize()
{
	struct map edit[1];
	size_t length;
	char *buffer;

	edit_ctor(edit);

	length = edit->length + 1;
	ok(buffer = malloc(length));
	memset(buffer, 'b', length);

	expect(0, edit_append(edit, buffer, length));
	expect(length, edit->offset);
	ok(!memcmp(edit->map, buffer, length));
}

int
main(int argc, char **argv)
{
	unit_parse_argv(argc, argv);
	return unit_run_tests(tests, arr_len(tests));
}
