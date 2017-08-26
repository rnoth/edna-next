#ifndef _edna_ln_
#define _edna_ln_

#include <stddef.h>
#include <ext.h>

int ln_insert(struct ext *lines, size_t start, char *buffer, size_t length);
void ln_delete(struct ext *lines, size_t offset, size_t extent);

#endif
