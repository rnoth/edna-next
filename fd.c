#include <errno.h>
#include <poll.h>
#include <sys/ioctl.h>

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

	return true;
}
