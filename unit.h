#ifndef _edna_unit_
#define _edna_unit_

#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <signal.h>

#define unit_catch() sigsetjmp(unit_checkpoint, 1)

#define unit_error_fmt(...) do {         \
	char buf[256];                   \
	snprintf(buf, 256, __VA_ARGS__); \
	unit_error(buf);                 \
} while (0)

#define unit_fail_fmt(...) do {          \
	char buf[256];                   \
	snprintf(buf, 256, __VA_ARGS__); \
	unit_fail(buf);                  \
} while (0)

#define unit_list(...) ((void (*[])()){__VA_ARGS__, 0x0})

#define try(EXPR) do {                 \
	unit_set_expr(#EXPR,__LINE__); \
	EXPR;                          \
	unit_unset_expr();             \
} while (0)

#define ok(EXPR) okf(EXPR, "assertion false: \"%s\" ", #EXPR); 

#define okm(EXPR, MSG) okf(EXPR, MSG "%s", "")
#define okf(EXPR, FMT, ...) do {	  \
	char *unit_msg = asprintf(FMT " (line %d)", \
	                          __VA_ARGS__, __LINE__); \
	_unit_ok(EXPR, unit_msg);          \
} while (0)

#define _unit_ok(EXPR, MSG) do {        \
	bool unit_res;                  \
	unit_set_expr(#EXPR, __LINE__); \
	unit_res = !!(EXPR);            \
	if (!unit_res) {                \
		unit_fail(MSG);         \
		unit_yield();           \
	}                               \
	unit_unset_expr();              \
} while (0)

#define expect(VAL, EXPR) do {                           \
	unit_set_expr(#EXPR, __LINE__);                  \
	long unit_res=(EXPR);                            \
	long unit_val=(VAL);                             \
	if (unit_res != unit_val) {                      \
		raise(SIGTRAP);                          \
		unit_fail_fmt("expected %s, got %ld"     \
		              " (expr \"%s\", line %d)", \
			      #VAL, unit_res,            \
		              #EXPR, __LINE__);          \
		unit_yield();                            \
	}                                                \
	unit_unset_expr();                               \
} while (0)

struct unit_test {
	char *msg;
	void (**fun)();
	void *ctx;
};

extern int unit_opt_error_fd;
extern unsigned unit_opt_timeout;
extern unsigned unit_opt_flakiness;
extern unsigned unit_opt_test_num;
extern bool unit_failed;
extern bool unit_has_init;
extern jmp_buf unit_checkpoint;

int unit_init();
void unit_set_expr(char *, int);
void unit_unset_expr(void);

void unit_perror(char *);
void unit_error(char *);
void unit_fail(char *);
int unit_parse_argv(size_t argc, char **argv);
int unit_run_tests(struct unit_test *, size_t);
void unit_yield(void) __attribute__((noreturn));

#endif
