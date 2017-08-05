#ifndef _edna_unit_
#define _edna_unit_
#include <stdbool.h>
#include <stdio.h>
#include <signal.h>

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

#define try(EXPR) do {   \
	unit_set_expr(#EXPR,__LINE__); \
	EXPR;                 \
	unit_unset_expr();    \
} while (0)

#define ok(EXPR) okf(EXPR, "assertion false: \"%s\"", #EXPR); 

#define okf(EXPR,  ...) do {      \
	char msg[256];                   \
	snprintf(msg, 256, __VA_ARGS__); \
	_unit_ok(EXPR, msg);             \
} while (0)

#define _unit_ok(EXPR, MSG) do { \
	bool unit_res;           \
	unit_set_expr(#EXPR, __LINE__); \
	unit_res = !!(EXPR);     \
	if (!unit_res) {         \
		raise(SIGTRAP);  \
		unit_fail(MSG);  \
	}                        \
	unit_unset_expr();       \
} while (0)

#define expect(VAL, EXPR) do { \
	int unit_res;                                \
	unit_set_expr(#EXPR, __LINE__);              \
	unit_res = (EXPR);                           \
	if (unit_res != VAL) {                       \
		raise(SIGTRAP);                      \
		unit_fail_fmt("expected %s, got %d"  \
		              " (expr \"%s\", line %d)", \
			      #VAL, unit_res,        \
		              #EXPR, __LINE__);      \
	}                                            \
	unit_unset_expr();                           \
} while (0)

struct unit_test {
	char *msg;
	void (**fun)();
	void *ctx;
};

void unit_set_expr(char *, int);
void unit_unset_expr(void);

void unit_perror(char *);
void unit_error(char *);
void unit_fail(char *);
void unit_parse_argv(char **);
int  unit_run_tests(struct unit_test *, size_t);

#endif
