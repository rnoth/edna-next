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
#include <util.h>

#define rwritef(fd, ...) do { \
	char _msg[256]; \
	char _msg1[256]; \
	int _len = snprintf(_msg, 256, __VA_ARGS__); \
	expect(_len, write(fd, _msg, _len)); \
	msleep(1); \
	expect(_len, read(fd, _msg1, _len)); \
	okf(!strncmp(_msg, _msg1, _len), \
	    "expected input string to be echoed: %s\n", _msg); \
} while (false)

extern char **environ;

static void kill_edna();
static void expect_prompt();
static void expect_error();
static void insert_line(char *ln);
static void print_line(char *ln);
static void send_line(char *ln);
static void send_eof();
static void test_insert_eof();
static void test_insert0();
static void test_insert1();
static void spawn_edna();
static void quit_edna();
static void wait_edna();

struct unit_test tests[] = {
	{.msg = "should see a prompt",
	 .fun = unit_list(spawn_edna, expect_prompt, kill_edna),},
	{.msg = "should exit on eof",
	 .fun = unit_list(spawn_edna, send_eof, wait_edna),},
	{.msg = "should be able to quit",
	 .fun = unit_list(spawn_edna, quit_edna, wait_edna),},

	{.msg = "should accept empty lines",
	 .fun = unit_list(spawn_edna,
	                  expect_prompt, send_line,
	                  expect_prompt, quit_edna, wait_edna
	                  ),
	 .ctx = "",},
	{.msg = "should produce errors on unknown commands",
	 .fun = unit_list(spawn_edna,
	                  expect_prompt, send_line, expect_error,
	                  quit_edna, wait_edna),
	 .ctx = "unknown",},

	{.msg = "should read multiple lines",
	 .fun = unit_list(spawn_edna,
	                  expect_prompt, send_line, expect_error,
	                  expect_prompt, send_line, expect_error,
	                  expect_prompt, quit_edna, wait_edna),
	 .ctx = "hi hi",},

	{.msg = "should be able to exit insert mode with eof",
	 .fun = unit_list(spawn_edna, test_insert_eof,
	                  quit_edna, wait_edna),},

	{.msg = "should be able to insert lines",
	 .fun = unit_list(spawn_edna,
	                  expect_prompt, insert_line,
	                  expect_prompt, print_line,
	                  expect_prompt, quit_edna, wait_edna),
	 .ctx = "Hello, world!\n",},

	{.msg = "should be able to insert multiple lines",
	 .fun = unit_list(spawn_edna, test_insert0, quit_edna, wait_edna),},
	{.msg = "should be able to insert multiple lines seperately",
	 .fun = unit_list(spawn_edna, test_insert1, quit_edna, wait_edna),},
};

static pid_t edna_pid;
static int   edna_pty;

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
	int res;
	char buf[256]={0};

	msleep(1);
	ok(res = read(edna_pty, buf, 255));
	buf[res] = 0;
	okf(*buf == ':', "expected a prompt (':'), got \"%s\"", buf);
}

void
expect_error()
{
	char buffer[3] = {0};
	ssize_t res;
	
	ok(res = read(edna_pty, buffer, 2));
	okf(res > 0, "read failed");

	okf(!strncmp(buffer, "?\n", 2),
	    "expected \"?\\n\" on error, got %s",
	    buffer);
}

void
insert_line(char *ln)
{
	rwritef(edna_pty, "i\n");
	rwritef(edna_pty, "%s", ln);
	ok(write(edna_pty, "\x04", 1) == 1);
}

void
print_line(char *ln)
{
	char buf[256]={0};
	size_t len;

	len = strlen(ln);
	rwritef(edna_pty, "p\n");
	len = read(edna_pty, buf, len);

	okf(!strncmp(buf, ln, len), "expected \"%s\", got \"%s\"", ln, buf);
}

void
send_line(char *ln)
{
	rwritef(edna_pty, "%s\n", ln);
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
test_insert_eof()
{
	expect_prompt();
	send_line("i");
	dprintf(edna_pty, "\x04");
	expect_prompt();

	okf(!waitpid(edna_pid, 0, WNOHANG), "edna died unexpectedly");
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
	print_line("good day indeed");
}

void
test_insert1()
{
	expect_prompt();
	insert_line("one");
	expect_prompt();
	insert_line("two");
	expect_prompt();
	print_line("two");
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
