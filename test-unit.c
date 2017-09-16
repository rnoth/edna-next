#include <stdarg.h>
#include <sysexits.h>

#include <fd.h>
#include <util.h>

#include <unit.c>

static int pipefd[2];
static volatile size_t test_num=1;

static void print_test_error(char *err);
static void print_test_failure(char *fail);
static void print_test_message(char *msg);
static void test_signals(void);

void
print_test_error(char *err)
{
	dprintf(2, "error\n\t%s", err);
}

void
print_test_failure(char *fail)
{
	dprintf(2, "failed\n\t%s", fail);
}

void
print_test_message(char *msg)
{
	dprintf(2, "%2zu| %s...", test_num++, msg);
}

void
test_print(void)
{
	struct read result[1];
	int err;

	print_test_message("should print error messages");

	unit_opt_error_fd = pipefd[1];

	if (!unit_catch()) {
		unit_error("hello, world");
	}

	if (!fd_readable(pipefd[0])) {
		print_test_failure("failed to print anything");
		return;
	}

	err = fd_read(result, pipefd[0]);
	if (err) {
		print_test_error("failed to read from pipe");
		exit(EX_OSERR);
	}

	if (!strncmp(result->buffer, "error\n\thello, world", result->length)) {
		char *t = malloc(2 * result->length + 1);
		size_t i;

		for (i=0; i<result->length; ++i) {
			switch (result->buffer[i]) {
			case '\n':
				t[i++] = '\\';
				t[i] = 'n';
				continue;

			case '\t':
				t[i++] = '\\';
				t[i] = 't';
				continue;
			}

			t[i] = result->buffer[i];
		}

		t[i] = 0;
		char *s = asprintf("expected \"error\n\thello, world\""
		                   "got \"%s\"", t);
		print_test_failure(s);
		free(s);
		free(t);
		return;
	}

	write_str(2, "ok\n");
}

void
test_signals(void)
{
	struct { char *id; int sig; } signals[] = {
		{"segfaults", SIGSEGV, },
		{"illegal instructions", SIGILL, },
		{"bus errors", SIGBUS, },
		{"aborts", SIGABRT,},
		{"alarms", SIGALRM, },
	};
	volatile size_t i=0;
	int err;

	print_test_message("should catch deadly signals");

	if (unit_catch()) {
		
		++i;
		if (i >= arr_len(signals)) {
			write_str(2, "ok\n");
			return;
		}
	}

	err = raise(signals[i].sig);
	if (err) {
		perror("raise failed");
		exit(EX_OSERR);
	}
}

int
main(int argc, char **argv)
{
	int err;

	err = pipe(pipefd);
	if (err == -1) {
		print_test_error("failed to create pipe");
		exit(EX_OSERR);
	}

	err = unit_init();
	if (err) {
		printf("unit_init failed unexpectedly\n");
		exit(EX_SOFTWARE);
	}

	test_print();
	test_signals();
}
