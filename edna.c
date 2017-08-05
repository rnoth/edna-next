#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>

#include <edna.h>

struct command {
	char *name;
	int (*func)(struct edna *);
};

struct edna *edna_ctor(void);
struct piece *txt_ctor(void);
void edna_dtor(struct edna *);
void txt_dtor(struct piece *);

int
edna_init(struct edna *edna)
{
	edna->txt = txt_ctor();

	if (!edna->txt) return ENOMEM;

	return 0;
}

void
edna_fini(struct edna *edna)
{
	txt_dtor(edna->txt);
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

