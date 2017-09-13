#include <sysexits.h>
#include <unit.c>

static volatile size_t test_num;
static void test_signals(void);

void
test_signals(void)
{
	struct { char *id; int sig; } signals[] = {
		{"segfaults", SIGSEGV, },
		{"alarms", SIGALRM, },
		{"illegal instructions", SIGILL, },
		{"bus errors", SIGBUS, },
		{"traps", SIGTRAP, },
	};
	volatile size_t i=0;
	int err;

	err = unit_init();
	if (err) {
		printf("unit_init failed unexpectedly\n");
		exit(EX_SOFTWARE);
	}

	unit_catch {
		printf("ok\n");
		++i;
		if (i >= arr_len(signals)) {
			return;
		}
	}

	printf("%d| should catch %s", test_num, signals[i]->id);

	raise(signals[i]->sig);
}

int
main(int argc, char **argv)
{
	struct unit_context ctx[1];
	struct { char *id; int sig; } signals[] = {
		{"segfaults", SIGSEGV, },
		{"alarms", SIGALRM, },
		{"illegal instructions", SIGILL, },
		{"bus errors", SIGBUS, },
		{"traps", SIGTRAP, },
	};
	volatile size_t test_num_saved;
	volatile size_t test_num;

	unit_init(ctx);

	unit_catch() {
		printf("ok\n");
		if (i < arr_len(signals)) {
			goto again;
		}
	}

	printf("%d| should catch segfaults", i);

	raise(SIGSEGV);

	unit_init(ctx);

	ok_r()
	unit_parse_argv(2, (char *[]){"test-unit", "-a",});

	if (unit_opt_alarm) {
		printf("\n")
	} 
}
