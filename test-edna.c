#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <unit.h>
#include <util.h>

extern char **environ;

static void kill_edna();
static void expect_prompt();
static void insert_line();
static void print_line();
static void send_eof();
static void spawn_edna();
static void quit_edna();

struct unit_test tests[] = {
	{.msg = "should see a prompt",
	 .fun = unit_list(spawn_edna, expect_prompt, kill_edna),},
	{.msg = "should exit on eof",
	 .fun = unit_list(spawn_edna, send_eof),},
	{.msg = "should be able to quit",
	 .fun = unit_list(spawn_edna, quit_edna),},
	{.msg = "should be able to insert lines",
	 .fun = unit_list(spawn_edna,
	                  expect_prompt, insert_line,
	                  expect_prompt, print_line,
	                  expect_prompt, kill_edna),},
};

static pid_t edna_pid;
static int   edna_pty;

void
kill_edna()
{
	int res;
	int ws;

	msleep(1);
	res = waitpid(edna_pid, &ws, WNOHANG);
	if (res == -1) unit_perror("wait failed");
	else if (!res) {
		res = kill(edna_pid, SIGTERM);
		if (res) unit_perror("kill failed");
		return;
	}
	
	if (WIFSIGNALED(ws)) {
		unit_fail_fmt("edna exited abnormally: killed by signal %d",
		              WTERMSIG(ws));
	}

	if (WIFEXITED(ws)) {
		unit_fail_fmt("edna exited unexpected: exited with code %d",
			      WEXITSTATUS(ws));
	}
}

void
expect_prompt()
{
	int res;
	char buf[256];

	msleep(1);
	res = read(edna_pty, buf, 256);
	unit_ok_fmt(*buf == ':', "expected a prompt (':'), got %s", buf);
	if (res > 1) {
		unit_fail_fmt("unexpected trailing string after prompt: %s", buf+1);
	}
}

void
insert_line(char *ln)
{
	dprintf(edna_pty, "i\n%s\n.\n", ln);
}

void
print_line(char *ln)
{
	size_t len = strlen(ln)+1;
	char *buf;
	unit_ok(buf = malloc(len));
	read(edna_pty, buf, len-1);
	buf[len] = 0;

	unit_ok(!strcmp(buf, ln));
}

void
send_eof()
{
	dprintf(edna_pty, "\x04");
	unit_ok(waitpid(edna_pid, 0, 0));
}

void
quit_edna()
{
	dprintf(edna_pty, "q\n");
	unit_ok(waitpid(edna_pid, 0, 0));
}

void
spawn_edna()
{
	int fd[2];
	int res;

	res = pipe(fd);
	if (res) unit_perror("failed to create pipe");

	edna_pty = mk_pty();
	if (edna_pty == -1) unit_perror("couldn't alloc pty");

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

int
main(int argc, char **argv)
{
	unit_parse_args(argv);
	return unit_run_tests(tests, arr_len(tests));
}
