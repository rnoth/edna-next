#include <errno.h>
#include <stdlib.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <fd.h>
#include <util.h>

uint
fd_peek(int fd)
{
	uint nbytes;
	int err;

	err = ioctl(fd, FIONREAD, &nbytes);
	if (err == -1) return 0;

	return nbytes;
}

int
fd_read(struct read *dest, int fd)
{
	ssize_t res;
	int err;

	err = fd_wait(fd);
	if (err) return err;

	dest->length = fd_peek(fd);

	dest->buffer = malloc(dest->length);
	if (!dest->buffer) return errno;

	dest->offset = 0;

	res = read(fd, dest->buffer, dest->length);
	if (res == -1) return errno;
	if (res == 0) return -1;

	return 0;
}

bool
fd_readable(int fd)
{
	struct pollfd pollfd[1];
	int err;

	pollfd->fd = fd;
	pollfd->events = POLLIN;

	err = poll(pollfd, 1, 0);
	if (err == -1) return false;
	if (err == 0) return false;

	return true;
}

int
fd_wait(int fd)
{
	struct pollfd pollfd[1];
	int err;

	pollfd->fd = fd;
	pollfd->events = POLLIN;

	err = poll(pollfd, 1, -1);
	if (err == -1) return errno;

	return 0;
}
