#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include <cmd.h>
#include <edna.h>
#include <file.h>
#include <set.h>
#include <util.h>

enum error {
	err_unknown_command=-2,
};

struct file;

static int exec_ln();

static void setup(void);
static int run(void);
static void cleanup(void);

static struct set cmds[1];
static struct edna edna[1];

size_t
eat_ident(char *buffer, size_t length)
{
	size_t offset = 0;

	while (offset < length) {
		if (isspace(buffer[offset])) {
			break;
		} else ++offset;
	}

	return offset;
}

size_t
eat_spaces(char *buffer, size_t length)
{
	size_t offset = 0;

	while (offset < length) {
		if (!isspace(buffer[offset])) {
			break;
		} else ++offset;
	}

	return offset;
}

int
exec_ln(void)
{
	static char buffer[4096];
	struct command *cmd;
	size_t length;
	size_t extent;
	size_t offset;

	length = read(0, buffer, 4096);
	if (length == -1UL) return errno;
	if (length == 0) return -1;

	offset = eat_spaces(buffer, length);
	if (offset >= length) return 0;

	extent = eat_ident(buffer+offset, length-offset);

	cmd = cmd_lookup(cmds, buffer+offset, extent);
	if (!cmd) {
		dprintf(1, "?\n");
		return 0;
	}

	return cmd->fun(cmd->arg);
}

int
insert_ln(void)
{
	static char buffer[4096];
	ssize_t length;

	while (true) {
		length = read(0, buffer, 4096);
		if (length == -1) return errno;
		if (!length) return 0;
	}
}

void
setup(void)
{
	cmd_init(cmds);
	edna_init(edna);
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
