#ifndef _edna_err_
#define _edna_err_

static inline void edna_fail(struct edna *edna, char *message);

void
edna_fail(struct edna *edna, char *message)
{
	edna->errmsg = message;
}

#endif
