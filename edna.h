#ifndef _edna_
#define _edna_

#include <edit.h>
#include <set.h>

struct edna;

void edna_fini(struct edna *edna);
int edna_init(struct edna *edna);
int edna_file_open(struct edna *edna, size_t offset, char *fn);
int edna_text_delete(struct edna *edna, size_t offset, size_t extent);
int edna_text_insert(struct edna *edna, size_t offset, char *text, size_t length);

struct edna {
	struct record *hist;
	struct piece *txt;
	struct frag *ln;
	struct set cmds[1];
	struct map edit[1];
	struct map file[1];
	size_t dot[2];
	char *errmsg;
};

#endif
