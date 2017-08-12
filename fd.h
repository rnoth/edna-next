#ifndef _edna_file_
#define _edna_file_

#include <stdbool.h>
#include <stdio.h>

int file_peek(size_t *dest, int fd);
bool file_readable(int fd);

#endif
