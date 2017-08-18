#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include <edna.h>
#include <cmd.h>
#include <txt.h>
#include <util.h>
#include <vec.h>

struct record {
	struct record *prev;
	size_t length;
	struct {
		size_t length;
		char *buffer;
	} chain[];
};

static int rec_marshal(struct record **dest, struct piece *txt);

void
edna_fini(struct edna *edna)
{
	text_dtor(edna->chain);
	text_dtor(edna->dead);
}

int
edna_init(struct edna *edna)
{
	*edna = (struct edna){0};

	edna->chain = text_ctor();
	if (!edna->chain) return ENOMEM;

	*edna->cmds = (struct set){0};
	cmd_init(edna->cmds);

	return 0;
}

int
edna_text_delete(struct edna *edna, size_t offset, size_t extent)
{
	struct piece *links[2];
	int err;

	links[0] = edna->chain, links[1] = 0;
	err = text_delete(links, offset, extent);
	if (err) return err;

	if (links[0]) {
		text_link(links[0], edna->dead);
		edna->dead = links[0];
	}

	return 0;
}

int
edna_text_insert(struct edna *edna, size_t offset, char *text, size_t length)
{
	struct record *_;
	struct piece *links[2];
	struct piece *pie;
	int err;

	err = rec_marshal(&_, edna->chain);
	if (err > 0) return err;
	if (!err) free(_);

	pie = calloc(1, sizeof *pie);
	if (!pie) return ENOMEM;

	pie->buffer = text;
	pie->length = length;

	links[0] = edna->chain, links[1] = 0;

	err = text_insert(links, pie, offset);
	if (err) {
		free(pie);
		return err;
	}

	return 0;
}

int
rec_marshal(struct record **dest, struct piece *txt)
{
	struct piece *t[2];
	size_t i;

	t[0] = text_next(txt, 0), t[1] = txt;

	for (i=0; text_next(t[0], t[1]); ++i) text_step(t);
	if (!i) return -1;

	*dest = calloc(1, sizeof **dest + i * sizeof *dest[0]->chain);
	if (!*dest) return ENOMEM;

	dest[0]->length = i;

	ptr_swap(t+0, t+1);
	while (--i, text_next(t[0], t[1])) {
		text_step(t);
		dest[0]->chain[i].length = t[1]->length;
		dest[0]->chain[i].buffer = t[1]->buffer;
	}

	return 0;
}
