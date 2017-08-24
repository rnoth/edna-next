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

static int run(struct edna *edna);

static size_t cursor[2] = {0, 0};

#define cmd(n, f, a) { .node = {{.key = n}}, .fun = f, .arg = a }
static struct command commands[] = {
	cmd("q", edna_cmd_quit, 0),
	cmd("i", edna_cmd_insert, cursor),
	cmd("p", edna_cmd_print, cursor),
	cmd("-", edna_cmd_back, cursor),
	cmd("+", edna_cmd_forth, cursor),
};

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
	struct parse *parse = 0;;
	static char buffer[4096];
	ssize_t length;
	int err;

	err = write_str(1, ":");
	if (err < 0) return errno;

	length = read(0, buffer, 4096);
	if (!length) return -1;
	if (length < 0) return errno;

	err = parse_ln(&parse, buffer, length);
	if (err) goto done;

	err = exec_ln(edna, parse);
	if (err) goto done;

 done:
	free(parse);
	return err;
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

