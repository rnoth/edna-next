#include <unistd.h>

#include <unit.h>
#include <file.c>

static void create_handle(struct file *hand);
static void read_handle(struct file *hand);
static void read_buffered(struct file *hand);
static void test_discard(struct file *hand);

static struct file ghand[1];

struct unit_test tests[] = {
	{.msg = "should be able to create a handle",
	 .fun = unit_list(create_handle),
	 .ctx = ghand,},
	{.msg = "should be able to read from a handle",
	 .fun = unit_list(create_handle, read_handle),
	 .ctx = ghand,},
	{.msg = "should do buffered reads",
	 .fun = unit_list(create_handle, read_buffered),
	 .ctx = ghand,},
	{.msg = "should discard lines properly",
	 .fun = unit_list(create_handle, test_discard),
	 .ctx = ghand,},
};

static int pipe_fd[2];

void
create_handle(struct file *hand)
{
	try(file_init(hand, pipe_fd[0]));
}

void
read_buffered(struct file *hand)
{
	ok(dprintf(pipe_fd[1], "abc") > 0);

	ok(file_readable(hand));

	expect(0, hand->offset);
	expect(0, hand->length);
	ok(file_get_char(hand) == 'a');

	ok(!file_readable(hand));

	expect(1, hand->offset);
	expect(3, hand->length);
	ok(file_get_char(hand) == 'b');

	expect(2, hand->offset);
	expect(3, hand->length);
	ok(file_get_char(hand) == 'c');

	expect(0, hand->offset);
	expect(0, hand->length);
}

void
read_handle(struct file *hand)
{
	char buffer[16] = {0};

	ok(dprintf(pipe_fd[1], "doodly-do") > 0);
	ok(!file_read_into_buffer(hand, buffer, 15));

	ok(!strcmp(buffer, "doodly-do"));
}

void
test_discard(struct file *hand)
{
	char buffer[256] = {0};
	dprintf(pipe_fd[1], "me too thanks\n");

	ok(!file_discard_line(hand));
	ok(!file_readable(hand));

	dprintf(pipe_fd[1], "ssssssssss\nhere we are");
	ok(!file_discard_line(hand));
	ok(!file_read_into_buffer(hand, buffer, 255));
	ok(!strcmp(buffer, "here we are"));
}

int
main(int argc, char **argv)
{
	unit_parse_argv(argv);

	ok(!pipe(pipe_fd));

	unit_run_tests(tests, sizeof tests/sizeof *tests);

	ok(!close(pipe_fd[0]));
	ok(!close(pipe_fd[1]));
}
