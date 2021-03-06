#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include <unit.h>
#include <fd.h>
#include <util.h>

#define insert_lines(...) _insert_lines((char *[]){__VA_ARGS__, 0})

#define argv(...) ((char *[]){__VA_ARGS__})

//static void create_test_file();
static void kill_edna();
static void expect_error();
static void expect_prompt();
static void _insert_lines(char **lns);
static void read_line(char *ln);
static void send_line(char *ln);
static void send_eof();

static void test_back();

static void test_delete_empty();
static void test_delete_end();
static void test_delete_one();

static void test_empty_line();
static void test_ends();

//static void test_file_arg();

static void test_forth();
static void test_forth_traverse();

static void test_insert_back();
static void test_insert_dot();
static void test_insert_eof();
static void test_insert_flat();
static void test_insert_empty();
static void test_insert_large();
static void test_insert_simple();
static void test_insert_succ_dec();
static void test_insert_three();
static void test_insert_two();
static void test_insert_seperate();

static void test_multiple_lines();
static void test_print_empty();
static void test_unknown_cmd();

static void spawn_edna(char **argv);
static void quit_edna();
static void wait_edna();

extern char **environ;

static pid_t edna_pid;
static int   edna_pty;
static int  temp_fd;
static char temp_file[] = "edna_temp_file";

#define edna_list(...) unit_list(spawn_edna, __VA_ARGS__, quit_edna)
struct unit_test tests[] = {
	{.msg = "should see a prompt",
	 .fun = unit_list(spawn_edna, expect_prompt, kill_edna),},
	{.msg = "should exit on eof",
	 .fun = unit_list(spawn_edna, send_eof, wait_edna),},
	{.msg = "should quit",
	 .fun = unit_list(spawn_edna, quit_edna),},

	{.msg = "should accept empty lines",
	 .fun = edna_list(test_empty_line),},
	{.msg = "should produce errors on unknown commands",
	 .fun = edna_list(test_unknown_cmd),},

	{.msg = "should read multiple lines",
	 .fun = edna_list(test_multiple_lines),},

	{.msg = "should error when printing empty selections",
	 .fun = edna_list(test_print_empty),},

	{.msg = "should exit insert mode with eof",
	 .fun = edna_list(test_insert_eof),},
	{.msg = "should exit insert mode with dot",
	 .fun = edna_list(test_insert_dot),},

	{.msg = "should insert lines",
	 .fun = edna_list(test_insert_simple),},

	{.msg = "should insert two lines",
	 .fun = edna_list(test_insert_two),},
	{.msg = "should insert lines seperately",
	 .fun = edna_list(test_insert_seperate),},
	{.msg = "should insert three lines",
	 .fun = edna_list(test_insert_three),},

	{.msg = "should handle empty lines",
	 .fun = edna_list(test_insert_empty),},

	{.msg = "should accept text without newlines",
	 .fun = edna_list(test_insert_flat),},
	{.msg = "should move backwards",
	 .fun = edna_list(test_back),},
	{.msg = "should insert anywhere",
	 .fun = edna_list(test_insert_back),},

	{.msg = "should move forwards",
	 .fun = edna_list(test_forth),},
	{.msg = "should move forwards multiple times",
	 .fun = edna_list(test_forth_traverse),},

	{.msg = "should error when moving to ends of buffer",
	 .fun = edna_list(test_ends),},

	{.msg = "should error when deleting empty selections",
	 .fun = edna_list(test_delete_empty),},
	{.msg = "should delete the only line in the buffer",
	 .fun = edna_list(test_delete_one),},
	{.msg = "should delete text at the end of buffer",
	 .fun = edna_list(test_delete_end),},

	{.msg = "should expand the edit buffer as necessary",
	 .fun = edna_list(test_insert_large),},

	{.msg = "should not hang on successive lines of decreasing length",
	 .fun = edna_list(test_insert_succ_dec),},

	/* {.msg = "should read file arguments", */
	/*  .fun = unit_list(create_test_file, */
	/*                   spawn_edna, test_file_arg, quit_edna), */
	/*  .ctx = argv("edna", temp_file),}, */
};

void
create_test_file(void)
{
	int nlines;
	int nchars;
	int i;
	int j;
	char c;

	temp_fd = open(temp_file, O_RDWR | O_TRUNC | O_CREAT);

	nlines = rand() % 20;
	for (i=0; i<nlines; ++i) {
		nchars = rand() % 80;
		for (j=0; j<nchars; ++j) {
			c = rand() % 'z' - 'A';
			c += 'A';
			if (write(temp_fd, &c, 1) != 1) {
				unit_perror("write failed");
			}
		}

		if (write(temp_fd, "\n", 1) != 1) {
			unit_perror("write failed");
		}
	}
}

void
rwritef(char *fmt, ...)
{
	va_list args;
	size_t res;
	size_t len;
	int ws;
	char *s;
	char *t;

	va_start(args, fmt);
	len = vsnprintf(0, 0, fmt, args);
	ok(s = calloc(len + 2, 1));
	ok(t = calloc(len + 2, 1));
	va_start(args, fmt);
	ok(vsnprintf(s, len + 1, fmt, args));
	va_end(args);

	ok(write(edna_pty, s, len));
	msleep(2);

	try(res = read(edna_pty, t, len));
	if (res == len && !strncmp(s, t, len)) {
		free(s);
		free(t);
		return;
	}

	msleep(1);
	if (!waitpid(edna_pid, &ws, WNOHANG)) {
		kill_edna();
		unit_fail_fmt("expected input string to be echoed: %s, "
		              "instead got %s\n", s, t);
		free(s), free(t);
		unit_yield();
	}

	if (WIFSIGNALED(ws)) {
		unit_fail_fmt("edna died unexpectedly (killed by signal %d)",
		              WTERMSIG(ws));
		unit_yield();
	}

	if (WIFEXITED(ws)) {
		unit_fail_fmt("edna exited unexpectedly with code %d",
		              WEXITSTATUS(ws));
		unit_yield();
	}
}

void
readf(char *fmt, ...)
{
	va_list args;
	va_list args1;
	size_t len;
	size_t avail;
	int ws;
	char *s;
	char *t;

	msleep(1);

	va_start(args, fmt);
	va_copy(args1, args);
	len = vsnprintf(0, 0, fmt, args);
	ok(s = calloc(len + 1, 1));
	ok(vsnprintf(s, len + 1, fmt, args1));

	va_end(args);
	va_end(args1);

	avail = fd_peek(edna_pty);
	ok(t = calloc(avail, 1));

	ok(read(edna_pty, t, avail) == (ssize_t)avail);

	if (avail == len && !strncmp(s, t, len)) {
		free(s), free(t);
		return;
	}

	if (!waitpid(edna_pid, &ws, WNOHANG)) {
		kill_edna();
		if (!*t) unit_fail_fmt("expected \"%s\", instead edna hangs",s);
		else unit_fail_fmt("expected \"%s\", got \"%s\"", s, t);
		free(s), free(t);
		unit_yield();
	}

	free(s), free(t);

	if (WIFSIGNALED(ws)) {
		unit_fail_fmt("edna died unexpectedly (killed by signal %d)",
		              WTERMSIG(ws));
		unit_yield();
	}

	if (WIFEXITED(ws)) {
		unit_fail_fmt("edna exited unexpectedly with code %d",
		              WEXITSTATUS(ws));
		unit_yield();
	}
}

void
kill_edna()
{
	int res;

	res = kill(edna_pid, SIGTERM);
	if (res) unit_perror("kill failed");
	return;
}

void
expect_error(char *msg)
{
	read_line("?");
	send_line("h");
	read_line(msg);
}

void
expect_prompt()
{
	readf(":");
}

void
_insert_lines(char **lns)
{
	send_line("i");
	while (*lns) rwritef("%s\n", *lns++);
	send_eof();
}

void
read_line(char *ln)
{
	readf("%s\n" ":", ln);
}

void
send_cmd(char *cmd)
{
	rwritef("%s\n", cmd);
	readf(":");
}

void
send_line(char *ln)
{
	rwritef("%s\n", ln);
	msleep(1);
}

void
send_eof()
{
	dprintf(edna_pty, "\x04");
}

void
quit_edna()
{
	dprintf(edna_pty, "q\n");
	wait_edna();
}

void
spawn_edna(char **argv)
{
	struct termios tattr[1];
	int fd[2];
	int res;

	tcflush(edna_pty, TCIOFLUSH);
	res = pipe(fd);
	if (res) unit_perror("failed to create pipe");

	tcgetattr(edna_pty, tattr);
	tattr->c_oflag &= ~OPOST;
	tcsetattr(edna_pty, TCSANOW, tattr);

	res = fork();
	switch (res) {
	case -1: unit_perror("fork failed"); unit_yield();
	case 0:
		open_pty(edna_pty); // FIXME: this could fail
		close(fd[0]);
		fcntl(fd[1], F_SETFD, FD_CLOEXEC);
		execve("./edna", argv ? argv : (char*[]){"edna",0}, environ);
		write(fd[1], (char[]){0}, 1);
		_exit(1);

	default: 
		edna_pid = res;
		break;
	}
	
	close(fd[1]);

	res = read(fd[0], (char[]){0}, 1);
	switch (res) {
	case -1: unit_perror("internal read failed"); unit_yield();
	case  1: unit_fail("couldn't exec edna"); unit_yield();
	case  0: break;
	}
	close(fd[0]);
}

void
test_back()
{
	expect_prompt();
	insert_lines("one", "two");

	expect_prompt();
	send_line("-");

	expect_prompt();
	send_line("p");
	read_line("one");
}

void
test_delete_empty()
{
	expect_prompt();
	send_line("d");
	expect_error("empty selection");
}

void
test_delete_end()
{
	expect_prompt();
	send_line("p");
	read_line("?");

	insert_lines("don't delete me!", "take me instead!");

	expect_prompt();
	send_line("d");
	expect_prompt();
	send_line("p");
	read_line("don't delete me!");
}

void
test_delete_one()
{
	expect_prompt();
	insert_lines("tell my wife I love her");

	expect_prompt();
	send_line("d");

	expect_prompt();
	send_line("p");
	read_line("?");
}

void
test_empty_line()
{
	expect_prompt();
	send_line("");
	expect_prompt();
}

void
test_ends()
{
	expect_prompt();
	send_line("-");
	expect_error("beginning of file");
	send_line("+");
	expect_error("end of file");
}

void
test_file_arg()
{
	size_t offset=0;
	size_t extent=0;
	off_t len;
	char *map;

	len = lseek(temp_fd, SEEK_END, 0);
	if (len == -1) unit_perror("lseek failed");

	map = mmap(0, len, PROT_READ, MAP_PRIVATE, temp_fd, 0);
	if (!map) unit_perror("mmap failed");

	expect_prompt();
	while ((offset+=extent)<(size_t)len) {
		try(extent = next_line(map+offset, len-offset));
		send_line("p");
		readf("%*s\n" ":", extent, map+offset);
	}
}

void
test_forth()
{
	expect_prompt();
	insert_lines("hi again", "back yet?");

	expect_prompt();
	send_line("-");
	expect_prompt();
	send_line("p");
	read_line("hi again");

	send_line("+");	
	expect_prompt();
	send_line("p");
	read_line("back yet?");
}

void
test_forth_traverse()
{
	expect_prompt();
	insert_lines("eeny", "meeny", "miny", "moe");

	expect_prompt();
	send_cmd("-");
	send_cmd("-");
	send_cmd("-");

	rwritef("p\n");
	read_line("eeny");

	send_cmd("+");

	rwritef("p\n");
	read_line("meeny");

	send_cmd("+");

	rwritef("p\n");
	read_line("miny");

	send_cmd("+");

	rwritef("p\n");
	read_line("moe");
}

void
test_insert_back()
{
	expect_prompt();
	insert_lines("above", "below");

	expect_prompt();
	send_line("-");
	expect_prompt();
	insert_lines("in-between");

	expect_prompt();
	send_line("p");
	read_line("in-between");
}

void
test_insert_dot()
{
	expect_prompt();
	send_line("i");
	send_line(".");
	expect_prompt();
}

void
test_insert_eof()
{
	expect_prompt();
	send_line("i");
	send_eof();
	expect_prompt();

	okm(!waitpid(edna_pid, 0, WNOHANG), "edna died unexpectedly");
}

void
test_insert_flat()
{
	expect_prompt();
	send_line("i");
	rwritef("hi");
	send_eof();
	send_eof();
	expect_prompt();
	send_line("p");
	readf("hi" ":");
}

void
test_insert_empty()
{
	expect_prompt();
	insert_lines("");
	expect_prompt();
	send_line("p");
	read_line("");
}

void
test_insert_large()
{
	char *buffer;
	size_t i;
	char c=' ';

	ok(buffer = malloc(4001));

	for (i=0; i<4000; ++i) {
		buffer[i] = c;
		++c;
		if (c>'~') c='a';
	}

	buffer[4000] = 0;

	expect_prompt();
	insert_lines(buffer, buffer);

	expect_prompt();
	send_line("p");
	read_line(buffer);

	send_line("-");
	expect_prompt();
	send_line("p");
	read_line(buffer);
}

void
test_insert_simple()
{
	expect_prompt();
	insert_lines("Hello, world!");
	expect_prompt();
	send_line("p");
	read_line("Hello, world!");
}

void
test_insert_succ_dec()
{
	expect_prompt();
	insert_lines("one two three", "one two", "one");
	expect_prompt();
}

void
test_insert_seperate()
{
	expect_prompt();
	insert_lines("one");
	expect_prompt();
	insert_lines("two");
	expect_prompt();
	send_line("p");
	read_line("two");
}

void
test_insert_three()
{
	expect_prompt();
	insert_lines("hi", "how are you?", "I'm great thanks for asking");
	expect_prompt();
	send_line("p");
	read_line("I'm great thanks for asking");
}

void
test_insert_two()
{
	expect_prompt();
	insert_lines("good day sir", "good day indeed");
	expect_prompt();
	send_line("p");
	read_line("good day indeed");
}

void
test_multiple_lines()
{
	expect_prompt();
	send_line("one");
	read_line("?");
	send_line("two");
	read_line("?");
}

void
test_print_empty()
{
	expect_prompt();
	send_line("p");
	expect_error("empty selection");
}

void
test_unknown_cmd()
{
	expect_prompt();
	send_line("hi");
	read_line("?");
	send_line("h");
	read_line("unknown command");
}

void
wait_edna()
{
	int res;
	int ws;

	msleep(1);
	res = waitpid(edna_pid, &ws, WNOHANG);
	if (res == -1) unit_perror("wait failed");

	if (!res) {
		unit_fail("edna unexpectedly alive"), unit_yield();
	}

	if (WIFSIGNALED(ws)) {
		unit_fail_fmt("edna exited abnormally: killed by signal %d",
		              WTERMSIG(ws));
		unit_yield();
	}
}

int
main(int argc, char **argv)
{
	srand(time(0));

	edna_pty = mk_pty();
	if (edna_pty == -1) unit_perror("couldn't alloc pty");

	unit_opt_flakiness = 10;
	unit_parse_argv(argc, argv);
	unit_run_tests(tests, arr_len(tests));
	system("pkill -x edna");

	return 0;
}
