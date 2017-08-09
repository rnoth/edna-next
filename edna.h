#ifndef _edna_
#define _edna_

#include <set.h>

struct edna;

void edna_fini(struct edna *edna);
int edna_init(struct edna *edna);
int edna_text_insert(struct edna *edna, size_t offset, char *text, size_t length);

struct edna {
	struct piece *text;
	struct set cmds[1];
};

#endif
