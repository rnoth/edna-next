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
#include <set.h>
#include <util.h>

static int run(struct edna *edna);

int
run(struct edna *edna)
{
	static char buffer[4096];
	struct parse *parse;
	size_t length;
	int err;

	err = dprintf(1, ":");
	if (err < 0) return errno;

	length = read(0, buffer, 4096);
	if (length == -1UL) return errno;
	if (length == 0) return -1;

	err = parse_ln(&parse, buffer, length);
	if (err) return err;

	err = exec_ln(edna, parse);
	if (err) return err;

	return 0;
}

int
main()
{
	struct edna edna[1];
	int err = 0;

	edna_init(edna);
	
	while (!err) err = run(edna);

	edna_fini(edna);
}

