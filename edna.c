#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include <edna.h>
#include <txt.h>

void
edna_fini(struct edna *edna)
{
	text_dtor(edna->text);
}

int
edna_init(struct edna *edna)
{
	edna->text = text_ctor();
	if (!edna->text) return ENOMEM;

	return 0;
}

int
edna_text_insert(struct edna *edna, size_t offset, char *text, size_t length)
{
	struct piece *links[2];
	struct piece *pie;

	pie = calloc(1, sizeof *pie);
	if (!pie) return ENOMEM;

	pie->buffer = text;
	pie->length = length;

	links[0] = edna->text, links[1] = 0;
	offset = text_walk(links, offset);
	text_insert(links, pie, offset);

	return 0;
}

