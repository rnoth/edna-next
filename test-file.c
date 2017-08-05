#include <unistd.h>

#include <unit.h>
#include <file.c>

static void create_handle(struct file *hand);
static void read_handle(struct file *hand);
static void read_buffered(struct file *hand);

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
	ok(0);
}

void
read_handle(struct file *hand)
{
	char buffer[16] = {0};

	ok(dprintf(pipe_fd[1], "doodly-do") > 0);
	ok(!file_read_into_buffer(hand, buffer, 15));

	ok(!strcmp(buffer, "doodly-do"));
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
