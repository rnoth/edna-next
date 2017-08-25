#ifndef _edna_
#define _edna_

#include <ext.h>
#include <set.h>

struct edna;

void edna_fini(struct edna *edna);
int edna_init(struct edna *edna);
int edna_text_delete(struct edna *edna, size_t offset, size_t extent);
int edna_text_insert(struct edna *edna, size_t offset, char *text, size_t length);

struct map {
	size_t offset;
	size_t length;
	char *map;
	int fd;
};

struct edna {
	struct record *hist;
	struct piece *chain;
	struct ext lines[1];
	struct set cmds[1];
	struct map edit[1];
	char *errmsg;
};

#endif
