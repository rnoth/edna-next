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

static struct read input[1];
static int run(struct edna *edna);

int
run(struct edna *edna)
{
	struct parse *parse = 0;;
	static char buffer[4096];
	ssize_t length;
	int err;

	err = dprintf(1, ":");
	if (err < 0) return errno;

	length = read(0, buffer, 4096);
	if (!length) return -1;
	if (length < 0) return errno;

	err = parse_ln(&parse, buffer, length);
	if (err) goto done;

	err = exec_ln(edna, parse);
	if (err) goto done;

 done:
	free(input->buffer), *input = (struct read){0};
	free(parse);
	return err;
}

int
main()
{
	struct edna edna[1];
	int err = 0;

	if (!isatty(0)) {
		dprintf(2, "fatal: stdin is not a terminal\n");
		exit(EX_USAGE);
	}

	edna_init(edna);
	
	while (!err) err = run(edna);

	edna_fini(edna);
}

