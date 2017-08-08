#ifndef _edna_
#define _edna_

struct edna;

int edna_init(struct edna *edna);
void edna_fini(struct edna *edna);

struct edna {
	struct piece *text;
};

#endif
