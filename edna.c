#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>

#include <edna.h>

struct edna {
	struct piece *txt;
};

struct piece {
	struct piece *next;
	struct piece *prev;
	char  *str;
	size_t len;
};

struct command {
	char *name;
	int (*func)(struct edna *);
};

struct edna *edna_ctor(void);
struct piece *txt_ctor(void);
void edna_dtor(struct edna *);
void txt_dtor(struct piece *);

struct edna *
edna_ctor(void)
{
	struct edna *ret;

	ret = malloc(sizeof *ret);
	if (!ret) return 0;

	ret->txt = txt_ctor();
	if (!ret->txt) {
		free(ret);
		return 0;
	}

	return ret;
}

void
edna_dtor(struct edna *ctx)
{
	txt_dtor(ctx->txt);
	free(ctx);
}

struct piece *
txt_ctor(void)
{
	struct piece *ret;

	ret = calloc(1, sizeof *ret);
	if (!ret) return 0;

	ret->next = calloc(1, sizeof *ret->next);
	if (!ret->next) {
		free(ret);
		return 0;
	}

	return ret;
}

void
txt_dtor(struct piece *txt)
{
	struct piece *next;

	next = txt->next;
	while (next) free(txt), txt = next, next = txt->next;
}
