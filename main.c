#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include <edna.h>
#include <file.h>
#include <util.h>

enum error {
	err_unknown_command=-2,
};

struct file;

static int exec_ln();

static void setup(void);
static int run(void);
static void cleanup(void);

static struct edna edna[1];
static struct file input[1];

int
exec_ln(void)
{
	char ch;

	do { // FIXME: not unicode-aware
		ch = file_get_char(input);
		if (ch < 0) return -ch;
		if (ch == '\n') return 0;
	} while (isspace(ch));

	switch (ch) {
	case 'q':
		return -1;
	default:
		file_discard_line(input);
		dprintf(1, "?\n");
		return 0;
	}
}

void
setup(void)
{
	edna_init(edna);
	file_init(input, 0);
}

int
run(void)
{
	int err;

	err = dprintf(1, ":");
	if (err < 0) return errno;

	err = exec_ln();
	if (err) return err;

	return 0;
}

void
cleanup(void)
{
	edna_fini(edna);
}

int
main()
{
	int err = 0;

	setup();
	
	while (!err) err = run();

	cleanup();
}
