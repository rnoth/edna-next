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
	int err;

	err = dprintf(1, ":");
	if (err < 0) return errno;

	err = fd_read(input, 0);
	if (err) goto done;

	err = parse_ln(&parse, input->buffer, input->length);
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

	edna_init(edna);
	
	while (!err) err = run(edna);

	edna_fini(edna);
}

