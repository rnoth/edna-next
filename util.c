#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <util.h>

int
mk_pty(void)
{
	int err;
	int fd;

	fd = posix_openpt(O_RDWR);
	if (fd == -1) return -1;
	
	err = grantpt(fd);
	if (err) return -1;
	
	err = unlockpt(fd);
	if (err) return -1;

	return fd;
}

int
open_pty(int fd)
{
	char *pty;
	int err;
	
	pty = ptsname(fd);
	
	if (close(0)) return -1;
	if (close(1)) return -1;
	if (close(2)) return -1;

	err = open(pty, O_RDONLY);
	if (err == -1) return -1;
	
	err = open(pty, O_WRONLY);
	if (err == -1) return -1;
	
	err = open(pty, O_WRONLY);
	if (err == -1) return -1;

	return 0;
}

int
msleep(size_t t)
{
	return nanosleep((struct timespec[]){0, t * 1000 * 1000}, 0);
}
