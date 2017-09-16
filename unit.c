#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include <util.h>
#include <unit.h>

static int  trap_failures(void);
static void report_error(int);
static void run_test(struct unit_test *);

bool unit_has_init;
bool unit_failed;
int unit_opt_error_fd=2;
unsigned unit_opt_timeout = 1;
unsigned unit_opt_test_num = 0;
unsigned unit_opt_flakiness = 0;
jmp_buf unit_checkpoint;

static int line_number;
static char current_expr[256];
static char error_message[256];

int
trap_failures(void)
{
	int signals[] = { SIGSEGV, SIGALRM, SIGABRT, SIGBUS, SIGILL, };
	struct sigaction sa[1] = {0};
	size_t i;
	int err;

	sa->sa_handler = report_error;

	for (i=0; i<arr_len(signals); ++i) {
		err = sigaction(signals[i], sa, 0x0);
		if (err) {
			err = errno;
			write_str(2, "fatal: sigaction failed");
			return err;
		}
	}

	sa->sa_handler = SIG_IGN;
	err = sigaction(SIGTRAP, sa, 0x0);
	if (err) {
		err = errno;
		write_str(2, "fatal: sigaction failed");
		return err;
	}

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
	case SIGILL:
		why = "illegal instruction";
		break;
	case SIGBUS:
		why = "bus error";
		break;
	case SIGTRAP:
	default:
		why = "trapped";
		break;
	}

	dprintf(unit_opt_error_fd,
	        "error\n    %s executing: %s ", why, current_expr);
	if (line_number > 0) dprintf(unit_opt_error_fd, "(line %d)", line_number);
	else dprintf(unit_opt_error_fd,
	             "(line >%d)", -line_number);
	dprintf(unit_opt_error_fd, "\n");

	siglongjmp(unit_checkpoint, 1);
}

void
run_test(struct unit_test *test)
{
	size_t i;

	write_str(unit_opt_error_fd, test->msg);
	write_str(unit_opt_error_fd, "...");

	for (i=0; test->fun[i]; ++i) {
		alarm(unit_opt_timeout);
		test->fun[i](test->ctx);
		alarm(0);
	}

	write_str(unit_opt_error_fd, "ok\n");
	return;
}

void
unit_error(char *error)
{
	write_str(unit_opt_error_fd, "error\n\t");
	write_str(unit_opt_error_fd, error);
	write_str(unit_opt_error_fd, "\n");

	siglongjmp(unit_checkpoint, 1);
}

void
unit_perror(char *blame)
{
	char buffer[256];

	snprintf(buffer, 256, "%s: %s", blame, strerror(errno));

	unit_error(buffer);
}

int
unit_init()
{
	int err;

	if (unit_has_init) return 0;

	err = trap_failures();
	if (err) return err;

	unit_has_init = true;

	return 0;
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
	line_number = -line_number;
}

void
unit_fail(char *msg)
{
	snprintf(error_message, 256, "%s", msg);
	raise(SIGTRAP);
}

int
parse_arg(char **argv)
{
	char *rem=*argv;
	int nused=1;

	switch (argv[0][1]) {
	case 'a':
		unit_opt_timeout = 0;
		break;

	case 'f':
		++argv, rem = *argv;
		if (!*argv) goto badnum;

		unit_opt_flakiness = strtoul(*argv, &rem, 10);
		if (*rem) goto badnum;

		++nused;
		break;

	case 'n':
		++argv, rem = *argv;
		if (!*argv) goto badnum;

		unit_opt_test_num = strtoul(*argv, &rem, 10);
		if (*rem) goto badnum;

		++nused;
		break;

	case 't':
		++argv, rem = *argv;
		if (!*argv) goto badnum;

		unit_opt_timeout = strtoul(*argv, &rem, 10);
		if (*rem) goto badnum;

		++nused;
		break;

	default:
		write_str(1, "unknown option: ");
		if (rem) write_str(1, rem);
		else write_str(1, "(null)");
		return -1;

	badnum:
		write_str(1, "error: invalid number: ");
		if (rem) write_str(1, rem);
		else write_str(1, "(null)\n");
		return -1;
	}

	return nused;
}

int
unit_parse_argv(size_t argc, char **argv)
{
	int res;

	if (argc < 2) return 0;

	++argv, --argc;
	while (argc > 0) {
		if (argv[0][0] != '-') break;

		res = parse_arg(argv);
		if (res == -1) return res;
		argv += res, argc -= res;
	}

	return 0;
}

int
unit_run_tests(struct unit_test *tl, size_t len)
{
	size_t f=unit_opt_flakiness;
	volatile size_t i;
	int width;
	int err;
	
	err = trap_failures();
	if (err) return err;

	i = 0;

	while (sigsetjmp(unit_checkpoint, 1)) {
		if (!f) {
			dprintf(unit_opt_error_fd,
			        "failed\n    %s\n", error_message);
			return -1;
		}
		dprintf(unit_opt_error_fd, "\r");
		--f;
	}

	width = snprintf(0, 0, "%zu", len);

	if (unit_opt_test_num) {
		run_test(tl + unit_opt_test_num - 1);
		return 0;
	}

	for (; i<len; ++i) {
		dprintf(unit_opt_error_fd, "%*zd| ", width, i+1);
		run_test(tl + i);
	}

	return 0;
}

void
unit_yield(void)
{
	siglongjmp(unit_checkpoint, 0);
}
