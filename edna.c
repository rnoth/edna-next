#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include <edna.h>
#include <cmd.h>
#include <txt.h>
#include <util.h>
#include <vec.h>

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
	struct piece *links[2];
	struct piece *pie;
	int err;

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
