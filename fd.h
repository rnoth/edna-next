#ifndef _edna_file_
#define _edna_file_

#include <stdbool.h>
#include <stdio.h>
#include <util.h>

uint fd_peek(int fd);
bool fd_readable(int fd);

#endif
