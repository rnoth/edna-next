#ifndef _edna_
#define _edna_

struct edna;

int edna_init(struct edna *edna);
void edna_fini(struct edna *edna);

struct edna {
	struct piece *txt;
};

struct piece {
	struct piece *next;
	struct piece *prev;
	char  *str;
	size_t len;
};

#endif
