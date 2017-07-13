#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <unit.h>
#include <sys/wait.h>

#include <util.h>

extern char **environ;

static void kill_edna();
static void spawn_edna();

struct unit_test tests[] = {
	{ .msg = "should launch normally",
	  .fun = unit_list(spawn_edna, kill_edna),
	},
};

static pid_t edna_pid;
static int   edna_pty;

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

void
kill_edna()
{
	int res;
	int ws;

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

int
main(int argc, char **argv)
{
	return unit_run_tests(argv, tests, arr_len(tests));
}
