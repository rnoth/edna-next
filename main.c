#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include <cmd.h>
#include <edna.h>
#include <exec.h>
#include <fd.h>
#include <set.h>
#include <util.h>

#define throw(lbl, err) do { \
	err = errno; \
	goto lbl;    \
} while (0)

static void init(struct edna *edna);
static int run(struct edna *edna);

static int cmd_quit();
static int cmd_help();

static char *errmsg;

#define cmd(n, f, a) { .node = {{.key = n}}, .fun = f, .arg = a }
static struct command commands[] = {
	cmd("q", cmd_quit, 0),
	cmd("h", cmd_help, 0),
	cmd("d", edna_cmd_delete, 0),
	cmd("a", edna_cmd_insert, "a"),
	cmd("c", edna_cmd_insert, "c"),
	cmd("i", edna_cmd_insert, "i"),
	cmd("p", edna_cmd_print, 0),
	cmd("-", edna_cmd_back, 0),
	cmd("+", edna_cmd_forth, 0),
};

int
cmd_help()
{
	int err;

	err = write_str(2, errmsg);
	if (err < 0) return errno;

	err = write_str(2, "\n");
	if (err < 0) return errno;

	return 0;
}

int
cmd_quit()
{
	return -1;
}

void
init(struct edna *edna)
{
	size_t i;
	int err;

	if (!isatty(0)) {
		write_str(2, "fatal: stdin is not a terminal\n");
		exit(EX_USAGE);
	}

	err = edna_init(edna);
	if (err) {
		write_str(2, "fatal: initialization failed: ");
		write_str(2, strerror(err));
		write_str(2, "\n");
		exit(EX_SOFTWARE);
	}

	for (i=0; i<arr_len(commands); ++i) {
		commands[i].len = strlen(commands[i].node->key) + 1;
		edna_add_cmd(edna, commands + i);
	}
}

int
run(struct edna *edna)
{
	struct parse parse[1];
	static char buffer[4096];
	ssize_t length;
	int err;

	err = write_str(1, ":");
	if (err < 0) return errno;

	length = read(0, buffer, 4096);
	if (!length) return -1;
	if (length < 0) return errno;

	err = parse_ln(parse, buffer, length);
	if (err) return err;

	err = exec_ln(edna, parse);
	if (err) return err;

	if (edna->errmsg) {
		errmsg = edna->errmsg;
		edna->errmsg = 0;
		err = write_str(2, "?\n");
		if (err) return errno;
	}

	return 0;
}

int
main()
{
	struct edna edna[1];
	int err = 0;

	init(edna);
	
	while (!err) err = run(edna);

	edna_fini(edna);
}

