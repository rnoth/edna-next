#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <mem.h>

int
memfd_create(char *name)
{
#ifdef SYS_memfd_create
	int fd;
	fd = syscall(SYS_memfd_create, name, 0);
	if (fd < 0) {
		errno = -fd;
		return -1;
	}

	fcntl(fd, F_SETFD, FD_CLOEXEC);
	return fd;
#else
	char buffer[256];
	int fd;

	snprintf(buffer, 256, "/tmp/%s-XXXXXX", name);

	fd = mkstemp(buffer);
	if (fd == -1) return -1;

	return unlink(buffer) > -1 ? fd : -1;
#endif
}

