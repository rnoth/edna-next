#include <stdlib.h>
#include <unistd.h>

#include <cmd.c>
#include <fd.h>
#include <txt.h>
#include <util.h>
#include <unit.h>

static void test_cmd_forth_eof(void);
static void test_cmd_insert(void);
static void test_cmd_print(void);

struct unit_test tests[] = {
	{.msg="should insert text",
	 .fun=unit_list(test_cmd_insert),},
	{.msg="should error when trying to move past eof",
	 .fun=unit_list(test_cmd_forth_eof),},
	{.msg="should print text",
	 .fun=unit_list(test_cmd_print),},
};

int pip[2];

#define UNIT_INIT { \
		ok(!pipe(pip)); \
		ok(!close(0));  \
		ok(!close(1));  \
		dup(pip[0]);    \
		dup(pip[1]);    \
	}
#include <unit.t>

void
readf(char *fmt, ...)
{
	va_list args;
	va_list args1;
	size_t len;
	size_t avail;
	char *s;
	char *t;

	va_start(args, fmt);
	va_copy(args1, args);
	len = vsnprintf(0, 0, fmt, args);
	ok(s = calloc(len + 1, 1));
	ok(vsnprintf(s, len + 1, fmt, args1));

	va_end(args);
	va_end(args1);

	avail = fd_peek(0);
	ok(t = calloc(avail, 1));

	ok(read(0, t, avail));

	if (avail == len && !memcmp(s, t, len)) {
		free(s), free(t);
		return;
	}

	unit_fail_fmt("expected \"%s\", got \"%s\"", s, t);
	free(s), free(t);
	unit_yield();
}

void
test_cmd_forth_eof(void)
{
	struct edna a[1];
	expect(0, edna_init(a));
	expect(0, cmd_forth(a));
	ok(a->errmsg);
}

void
test_cmd_insert(void)
{
	struct edna a[1];
	struct piece *p;
	expect(0, edna_init(a));

	write_str(1, "hi");
	expect(0, do_insert(a));
	ok(!memcmp(a->edit->map, "hi", 2));

	expect(2, a->ln->len);
	expect(0, a->dot[0]);
	expect(2, a->dot[1]);

	p = a->chain;
	p = untag(p->link);
	expect(0, p->offset);
	expect(2, p->length);
	ok(p->edit == a->edit);

	try(edna_fini(a));
}	

void
test_cmd_print(void)
{
	struct edna a[1];
	expect(0, edna_init(a));

	write_str(1, "hey");
	expect(0, do_insert(a));
	expect(0, cmd_print(a));

	readf("hey");
}
