#include <poll.h>
#include <sys/ioctl.h>

#include <fd.h>
#include <util.h>

int
fd_peek(size_t *dest, int fd)
{
	uint nbytes;
	int err;

	err = ioctl(fd, FIONREAD, &nbytes);
	if (err == -1) return err;

	*dest = nbytes;
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
