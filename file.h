#ifndef _edna_file_
#define _edna_file_

#include <stdbool.h>
#include <stdio.h>

struct file {
	char buffer[BUFSIZ];
	size_t offset;
	size_t length;
	int filedes;
};

void file_init(struct file *handle, int filedes);
int file_discard_line(struct file *handle);
int file_get_char(struct file *handle);
bool file_readable(struct file *handle);
int file_read_into_buffer(struct file *handle, char *buffer, size_t length);

#endif
