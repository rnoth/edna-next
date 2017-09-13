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
unsigned unit_opt_timeout = 1;
unsigned unit_opt_test_num = 0;
unsigned unit_opt_flakiness = 0;
static jmp_buf client_checkpoint;

static int line_number;
static char current_expr[256];
static char error_message[256];
static jmp_buf *escape_hatch;
static jmp_buf failure_hatch;
static jmp_buf exit_hatch;

#define throw(BLAME) do { int _ = errno; perror(BLAME); return _; } while (0)

int
trap_failures(void)
{
	struct sigaction sa[1] = {0};
	int err;

	sa->sa_handler = report_error;

	err = sigaction(SIGSEGV, sa, 0x0);
	if (err) throw("sigaction failed");

	err = sigaction(SIGALRM, sa, 0x0);
	if (err) throw("sigaction failed");

	err = sigaction(SIGBUS, sa, 0x0);
	if (err) throw("sigaction failed");

	err = sigaction(SIGILL, sa, 0x0);
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

	dprintf(2, "error\n    %s executing: %s ", why, current_expr);
	if (line_number > 0) dprintf(2, "(line %d)", line_number);
	else dprintf(2, "(line >%d)", -line_number);
	dprintf(2, "\n");
	exit(-1);
}

void
run_test(struct unit_test *te)
{
	size_t i;

	dprintf(2, "%s...", te->msg);

	if (setjmp(failure_hatch)) longjmp(exit_hatch, 0);

	for (i=0; te->fun[i]; ++i) {
		alarm(unit_opt_timeout);
		te->fun[i](te->ctx);
		alarm(0);
	}

	dprintf(2, "ok\n");
	return;
}

bool
unit_checkpoint(void)
{
	escape_hatch = &client_checkpoint;
	return !!setjmp(client_checkpoint);
}

void
unit_error(char *error)
{
	dprintf(2, "error\n    %s\n", error);
	if (!escape_hatch) {
		fprintf(stderr, "fatal: occured outside of unit_run_test() or unit_catch {}");
		exit(EX_SOFTWARE);
	}
	longjmp(*escape_hatch, 0);
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
	volatile int i_saved;
	size_t i;
	int width;
	int err;
	
	err = trap_failures();
	if (err) return err;

	i = 0;

	escape_hatch = &exit_hatch;
	while (setjmp(exit_hatch)) {
		if (!f) {
			dprintf(2, "failed\n    %s\n", error_message);
			return -1;
		}
		dprintf(2, "\r");
		--f;
		i = i_saved;
	}

	width = snprintf(0, 0, "%zu", len);

	if (unit_opt_test_num) {
		run_test(tl + unit_opt_test_num - 1);
		return 0;
	}

	for (; i<len; ++i) {
		i_saved = i;
		dprintf(2, "%*zd| ", width, i+1);
		run_test(tl + i);
	}

	return 0;
}

void
unit_yield(void)
{
	longjmp(failure_hatch, 0);
}
