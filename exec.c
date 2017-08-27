#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <cmd.h>
#include <exec.h>

static size_t eat_ident(char *buffer, size_t length);
static size_t eat_spaces(char *buffer, size_t length);

#define eat(RES, VAR, EXPR, BUF, LEN) do {  \
	size_t off=0;                       \
	while (off < (LEN)) {               \
		VAR = (BUF)[off];           \
		if (!(EXPR)) break;         \
		else ++off;                 \
	}                                   \
	(RES) = off;                        \
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
parse_ln(struct parse *parse, char *buffer, size_t length)
{
	size_t offset;
	size_t extent;

	*parse = (struct parse){0};
	offset = eat_spaces(buffer, length);
	if (offset >= length) return 0;

	extent = eat_ident(buffer+offset, length-offset);

	parse->string = buffer+offset;
	parse->length = extent;

	return 0;
}

int
exec_ln(struct edna *edna, struct parse *parse)
{
	struct command *cmd;

	if (!parse->length) return 0;

	cmd = edna_lookup_cmd(edna, parse->string, parse->length);
	if (!cmd) {
		edna->errmsg = "unknown command";
		return 0;
	}

	return cmd->fun(edna, cmd->arg);
}
