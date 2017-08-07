#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <set.h>

void
die(char *blame)
{
	perror(blame);
	exit(1);
}

void
run(char *map, size_t len)
{
	char *nl;
	size_t off;
	size_t ext;
	struct {
		struct set_node n[1];
		char s[];
	} *t;

	struct set s[1] = {{0}};

	for (off=0; off < len; off += ext) {
		nl = memchr(map+off, '\n', len - off);
		if (!nl) return;

		ext = nl - map - off + 1;
		t = malloc(sizeof *t + ext);
		if (!t) die("malloc failed");

		memcpy(t->s, map+off, ext);

		set_add(s, t->n, ext);
	}

	for (off=0; off < len; off += ext) {
		nl = memchr(map+off, '\n', len - off);
		if (!nl) return;

		ext = nl - map - off + 1;

		set_has(s, map+off, ext);
	}
}

int
main()
{
	off_t len;
	char *map;
	int fd;

	fd = open("words", O_RDONLY);
	if (fd == -1) die("open \"words\" failed");

	len = lseek(fd, 0, SEEK_END);
	if (len == -1) die("lseek failed");

	map = mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0);
	if (!map) die("mmap failed");

	run(map, len);

	munmap(map, len);
	close(fd);
}
