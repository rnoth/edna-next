#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <unit.h>

static int  trap_failures(void);
static void report_error(int);
static void report_failure();
static void run_test(struct unit_test *);

unsigned unit_opt_timeout = 1;
size_t unit_opt_test_num = 0;

static int line_number;
static char current_expr[256];
static char error_message[256];
static jmp_buf escape_hatch;
static jmp_buf checkpoint;

#define throw(BLAME) do { int _ = errno; perror(BLAME); return _; } while (0)

int
trap_failures(void)
{
	struct sigaction sa[1] = {0};
	int err;

	sa->sa_handler = report_failure;
	
	err = sigaction(SIGHUP, sa, 0x0);
	if (err) throw("sigaction failed");

	sa->sa_handler = report_error;

	err = sigaction(SIGSEGV, sa, 0x0);
	if (err) throw("sigaction failed");

	err = sigaction(SIGALRM, sa, 0x0);
	if (err) throw("sigaction failed");

	err = sigaction(SIGALRM, sa, 0x0);
	if (err) throw("sigaction failed");

	sa->sa_handler = SIG_IGN;
	err = sigaction(SIGTRAP, sa, 0x0);
	if (err) throw("sigaction failed");

	return 0;
}

void
report_error(int sig)
{
	char *why;
	switch (sig) {
	case SIGSEGV:
		why = "segfaulted";
		break;
	case SIGALRM:
		why = "timed out";
		break;
	case SIGABRT:
		why = "aborted";
		break;
	case SIGTRAP:
	default:
		why = "trapped";
		break;
	}

	dprintf(2, "error\n    %s executing: %s (line %d)\n",
	        why, current_expr, line_number);
	longjmp(escape_hatch, 0);
}

void
report_failure()
{
	longjmp(checkpoint, 0);
}

void
run_test(struct unit_test *te)
{
	size_t i;
	
	dprintf(2, "%s...", te->msg);

	for (i=0; te->fun[i]; ++i) {
		alarm(unit_opt_timeout);
		te->fun[i](te->ctx);
		alarm(0);
		if (setjmp(checkpoint)) goto failed;
	}

	dprintf(2, "ok\n");
	return;

 failed:
	dprintf(2, "failed\n    %s\n", error_message);
	longjmp(escape_hatch, 0);
}

void
unit_error(char *error)
{
	dprintf(2, "error\n    %s\n", error);
	longjmp(escape_hatch, 0);
}

void
unit_perror(char *blame)
{
	char buffer[256];

	snprintf(buffer, 256, "%s: %s", blame, strerror(errno));

	unit_error(buffer);
}

void
unit_set_expr(char *expr, int lineno)
{
	snprintf(current_expr, 256, "%s", expr);
	line_number = lineno;
}

void
unit_unset_expr(void)
{
	strcpy(current_expr, "(unknown expression)");
	line_number = -1;
}

void
unit_fail(char *msg)
{
	snprintf(error_message, 256, "%s", msg);
	raise(SIGHUP);
}

void
unit_parse_argv(char **argv)
{
	while (*++argv) {
		if (argv[0][0] != '-') break;
		switch (argv[0][1]) {
		case 'a':
			unit_opt_timeout = 0;
			break;
		case 'n':
			unit_opt_test_num = strtoul(*++argv, 0, 10);
			break;
		case 't':
			unit_opt_timeout = strtoul(*++argv, 0, 10);
			break;
		default:
			dprintf(2, "unknown option: %s (ignoring)", *argv);
		}
	}
}

int
unit_run_tests(struct unit_test *tl, size_t len)
{
	size_t i;
	int err;
	
	err = trap_failures();
	if (err) return err;

	if (setjmp(escape_hatch)) {
		return -1;
	}

	if (unit_opt_test_num) run_test(tl + unit_opt_test_num - 1);
	else for (i=0; i<len; ++i) run_test(tl + i);

	return 0;
}

