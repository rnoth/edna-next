#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>

#include <edna.h>

static int run(struct edna *);

int
main()
{
	struct edna *ctx;
	int err = 0;

	ctx = edna_ctor();
	
	while (!err) err = run(ctx);

	edna_dtor(ctx);
}

int
run(struct edna *ctx)
{
	static char buf[BUFSIZ];
	int err;

	dprintf(1, ":");

	err = read(0, buf, BUFSIZ);

	switch (err) {
	case -1:
		perror("read failed");
		exit(EX_OSERR);
	case 0:  return -1;
	default: break;
	}

	dprintf(1, "?\n");

	return 0;
}
