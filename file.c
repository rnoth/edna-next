#include <errno.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <file.h>
#include <util.h>

static int file_read(struct file *handle);

void
file_init(struct file *handle, int filedes)
{
	handle->filedes = filedes;
	handle->offset = 0;
	handle->length = 0;
}

int
file_read(struct file *handle)
{
	ssize_t result;

	handle->offset = 0;

	result = read(handle->filedes, handle->buffer, BUFSIZ);
	if (result < 0) return errno;

	handle->length = result;

	return 0;
}

size_t
file_copy_into_buffer(struct file *handle, char *buffer, size_t length)
{
	size_t min;
	size_t rem;

	rem = handle->length - handle->offset;

	min = umin(length, rem);
	memcpy(buffer, handle->buffer+handle->offset, min);

	handle->offset += min;

	if (handle->offset >= handle->length) {
		handle->offset = handle->length = 0;
	}

	return min;
}

int
file_discard_line(struct file *handle)
{
	int ch;

	do {
		ch = file_get_char(handle);
		if (ch == -1) return -1;
		if (ch < 0) return -ch;
	} while (ch != '\n');

	return 0;
}

int
file_get_char(struct file *handle)
{
	char res;
	int err;

	err = file_read_into_buffer(handle, &res, 1);
	if (err == -1) return -1;
	if (err) return -err;

	return res;
}

bool
file_readable(struct file *handle)
{
	struct pollfd pollfd[1];
	int err;

	pollfd->fd = handle->filedes;
	pollfd->events = POLLIN;

	err = poll(pollfd, 1, 0);
	if (err == -1) return false;
	if (err == 0) return false;

	return true;
}

int
file_read_into_buffer(struct file *handle, char *buffer, size_t length)
{
	size_t offset=0;
	int err;

	if (handle->length) {
		offset = file_copy_into_buffer(handle, buffer, length);
	}

	while (!offset || file_readable(handle)) {
		err = file_read(handle);
		if (err) return err;

		if (!handle->length) return -1;

		offset += file_copy_into_buffer(handle, buffer+offset,
		                                length - offset);
		if (length - offset == 0) break;
	}

	if (errno) return errno;

	return 0;
}

int
file_wait(struct file *handle)
{
	struct pollfd pollfd[1];

	pollfd->events = POLLIN;
	pollfd->fd = handle->filedes;

	return poll(pollfd, 1, -1) != -1 ? errno : 0;
}
