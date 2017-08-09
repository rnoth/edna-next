#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <cmd.h>

enum token {
	tok_nul,
	tok_cmd,
};

struct parse;

static size_t eat_ident(char *buffer, size_t length);
static size_t eat_spaces(char *buffer, size_t length);

struct parse {
	char *string;
	size_t length;
	enum token token;
	struct parse *chld[2];
};

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
parse_ln(struct parse **dest, char *buffer, size_t length)
{
	size_t offset;
	size_t extent;

	*dest = 0;

	offset = eat_spaces(buffer, length);
	if (offset >= length) return 0;

	*dest = malloc(sizeof *dest);
	if (!*dest) return ENOMEM;

	extent = eat_ident(buffer+offset, length-offset);

	dest[0]->token = tok_cmd;
	dest[0]->string = buffer+offset;
	dest[0]->length = extent;

	return 0;
}

int
exec_ln(struct edna *edna, struct parse *parse)
{
	struct command *cmd;

	cmd = cmd_lookup(edna->cmds, parse->string, parse->length);
	if (!cmd) {
		dprintf(1, "?\n");
		return 0;
	}

	return cmd->fun(edna, cmd->arg);
}
