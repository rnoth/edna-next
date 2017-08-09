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
	int err;

	err = dprintf(1, ":");
	if (err < 0) return errno;

	err = exec_ln(edna);
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

