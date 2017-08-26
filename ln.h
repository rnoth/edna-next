#ifndef _edna_ln_
#define _edna_ln_

int add_lines(struct ext *lines, size_t start, char *buffer, size_t length);
void rm_lines(struct ext *lines, size_t offset, size_t extent);

#endif
