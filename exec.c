#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#include <cmd.h>

static size_t eat_ident(char *buffer, size_t length);
static size_t eat_spaces(char *buffer, size_t length);

#define eat(RES, VAR, EXPR, BUF, LEN) do {	  \
	size_t off=0; \
	while (off < (LEN)) { \
		VAR = (BUF)[off]; \
		if (!(EXPR)) break; \
		else ++off; \
	} \
	(RES) = off; \
} while (0)

size_t
eat_ident(char *buffer, size_t length)
{
	size_t result;
	eat(result, char c, !isspace(c), buffer, length);
	return result;
}

size_t
eat_spaces(char *buffer, size_t length)
{
	size_t result;
	eat(result, char c, isspace(c), buffer, length);
	return result;
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
