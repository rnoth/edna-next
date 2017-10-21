#ifndef _edna_ln_
#define _edna_ln_

#include <stddef.h>
#include <frag.h>

int ln_insert(struct frag **lines, size_t start, char *buffer, size_t length);
void ln_delete(struct frag **lines, size_t offset, size_t extent);

#endif
