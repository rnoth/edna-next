#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#include <cmd.h>

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
exec_ln(struct edna *edna)
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

	cmd = cmd_lookup(edna->cmds, buffer+offset, extent);
	if (!cmd) {
		dprintf(1, "?\n");
		return 0;
	}

	return cmd->fun(edna, cmd->arg);
}
