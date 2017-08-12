#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include <unit.h>
#include <fd.h>
#include <util.h>

extern char **environ;

static void kill_edna();
static void expect_prompt();
static void insert_line(char *ln);
static void send_line(char *ln);
static void send_eof();

static void test_empty_line();

static void test_insert_dot();
static void test_insert_eof();
static void test_insert_empty();
static void test_insert_simple();
static void test_insert0();
static void test_insert1();

static void test_multiple_lines();
static void test_unknown_cmd();

static void spawn_edna();
static void quit_edna();
static void wait_edna();

struct unit_test tests[] = {
	{.msg = "should see a prompt",
	 .fun = unit_list(spawn_edna, expect_prompt, kill_edna),},
	{.msg = "should exit on eof",
	 .fun = unit_list(spawn_edna, send_eof, wait_edna),},
	{.msg = "should be able to quit",
	 .fun = unit_list(spawn_edna, quit_edna),},

	{.msg = "should accept empty lines",
	 .fun = unit_list(spawn_edna, test_empty_line, quit_edna),},
	{.msg = "should produce errors on unknown commands",
	 .fun = unit_list(spawn_edna, test_unknown_cmd, quit_edna),},

	{.msg = "should read multiple lines",
	 .fun = unit_list(spawn_edna, test_multiple_lines, quit_edna),},

	{.msg = "should be able to exit insert mode with eof",
	 .fun = unit_list(spawn_edna, test_insert_eof, quit_edna),},
	{.msg = "should be able to exit insert mode with dot",
	 .fun = unit_list(spawn_edna, test_insert_dot, quit_edna),},

	{.msg = "should be able to insert lines",
	 .fun = unit_list(spawn_edna,
	                  test_insert_simple,
	                  quit_edna),},

	{.msg = "should be able to insert multiple lines",
	 .fun = unit_list(spawn_edna, test_insert0, quit_edna),},
	{.msg = "should be able to insert multiple lines seperately",
	 .fun = unit_list(spawn_edna, test_insert1, quit_edna),},

	{.msg = "should handle empty lines properly",
	 .fun = unit_list(spawn_edna, test_insert_empty, quit_edna),},
};

static pid_t edna_pid;
static int   edna_pty;

void
rwritef(char *fmt, ...)
{
	va_list args;
	size_t len;
	char *s;
	char *t;

	va_start(args, fmt);
	len = vsnprintf(0, 0, fmt, args);
	ok(s = calloc(len + 1, 1));
	ok(t = calloc(len + 1, 1));
	va_start(args, fmt);
	ok(vsnprintf(s, len + 1, fmt, args));

	ok(write(edna_pty, s, len) == (ssize_t)len);
	msleep(1);

	ok(read(edna_pty, t, len) == (ssize_t)len);
	okf(!strncmp(s, t, len),
	    "expected input string to be echoed: %s, "
	    "instead got %s\n",
	    s, t);
	va_end(args);

	free(s);
	free(t);
}

void
readf(char *fmt, ...)
{
	va_list args;
	size_t len;
	size_t avail;
	int ws;
	char *s;
	char *t;

	msleep(2);

	va_start(args, fmt);
	len = vsnprintf(0, 0, fmt, args);
	ok(s = calloc(len + 1, 1));
	va_start(args, fmt);
	ok(vsnprintf(s, len + 1, fmt, args));

	avail = fd_peek(edna_pty);
	ok(t = calloc(avail, 1));

	ok(read(edna_pty, t, avail) == (ssize_t)avail);

	if (avail == len && !strncmp(s, t, len)) return;

	msleep(2);

	if (!waitpid(edna_pid, &ws, WNOHANG)) {
		unit_fail_fmt("expected \"%*s\", got \"%*s\"",
		              (int)len, s, (int)avail, t);
	}

	if (WIFSIGNALED(ws)) {
		unit_fail_fmt("edna died unexpectedly (killed by signal %d)",
		              WTERMSIG(ws));
	}

	if (WIFEXITED(ws)) {
		unit_fail_fmt("edna exited unexpectedly with code %d",
		              WEXITSTATUS(ws));
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
expect_prompt()
{
	readf(":");
}

void
insert_line(char *ln)
{
	rwritef("i\n");
	rwritef("%s\n", ln);
	ok(write(edna_pty, "\x04", 1) == 1);
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
spawn_edna()
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
	case -1: unit_perror("fork failed");
	case 0:
		open_pty(edna_pty); // FIXME: this could fail
		close(fd[0]);
		fcntl(fd[1], F_SETFD, FD_CLOEXEC);
		execve("./edna", (char*[]){"edna",0}, environ);
		write(fd[1], (char[]){0}, 1);
		_exit(1);

	default: 
		edna_pid = res;
		break;
	}
	
	close(fd[1]);

	res = read(fd[0], (char[]){0}, 1);
	switch (res) {
	case -1: unit_perror("internal read failed");
	case  1: unit_fail("couldn't exec edna");
	case  0: break;
	}
	close(fd[0]);
}

void
test_empty_line()
{
	expect_prompt();
	send_line("");
	expect_prompt();
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
	dprintf(edna_pty, "\x04");
	expect_prompt();

	okf(!waitpid(edna_pid, 0, WNOHANG), "edna died unexpectedly");
}

void
test_insert_empty()
{
	expect_prompt();
	send_line("i");
	send_line("");
	send_eof();
	expect_prompt();
	send_line("p");
	readf("\n" ":");
}

void
test_insert_simple()
{
	expect_prompt();
	send_line("i");
	send_line("Hello, world!");
	send_eof();
	expect_prompt();
	send_line("p");
	readf("Hello, world!\n" ":");
}

void
test_insert0()
{
	expect_prompt();
	send_line("i");
	send_line("good night sir");
	send_line("good day indeed");
	send_line(".");
	expect_prompt();
	send_line("p");
	readf("good day indeed\n" ":");
}

void
test_insert1()
{
	expect_prompt();
	insert_line("one");
	expect_prompt();
	insert_line("two");
	expect_prompt();
	send_line("p");
	readf("two\n" ":");
}

void
test_multiple_lines()
{
	expect_prompt();
	send_line("one");
	readf("?\n" ":");
	send_line("two");
	readf("?\n" ":");
}

void
test_unknown_cmd()
{
	expect_prompt();
	send_line("hi");
	readf("?\n" ":");
}

void
wait_edna()
{
	int res;
	int ws;

	msleep(10);
	res = waitpid(edna_pid, &ws, WNOHANG);
	if (res == -1) unit_perror("wait failed");

	if (!res) {
		unit_fail("edna unexpectedly alive");
	}

	if (WIFSIGNALED(ws)) {
		unit_fail_fmt("edna exited abnormally: killed by signal %d",
		              WTERMSIG(ws));
	}
}

int
main(int argc, char **argv)
{
	edna_pty = mk_pty();
	if (edna_pty == -1) unit_perror("couldn't alloc pty");

	unit_parse_argv(argv);
	return unit_run_tests(tests, arr_len(tests));
}
