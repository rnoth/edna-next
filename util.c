#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <util.h>

char *
asprintf(char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	return vasprintf(fmt, args);
}

char *
vasprintf(char *fmt, va_list args)
{
	va_list args2;
	size_t mem;
	char *res;

	va_copy(args2, args);

	mem = vsnprintf(0, 0, fmt, args);
	res = malloc(mem + 1);
	if (!res) return 0;

	vsnprintf(res, mem + 1, fmt, args2);

	return res;
}

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
