#ifndef _edna_file_
#define _edna_file_

#include <stdbool.h>
#include <stdio.h>
#include <util.h>

struct read {
	size_t length;
	size_t offset;
	char *buffer;
};

void file_init(int fd);
uint fd_peek(int fd);
int fd_read(struct read *dest, int fd);
bool fd_readable(int fd);
int fd_wait(int fd);

#endif
