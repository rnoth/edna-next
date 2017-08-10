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

size_t
wc(char *map, size_t len)
{
	size_t nlines=0;
	size_t off=0;
	char *nl;

	while (off < len) {
		nl = memchr(map+off, '\n', len-off);
		if (!nl) break;
		off = nl - map + 1;
		++nlines;
	}

	return nlines;
}

void
lb(size_t *offs, size_t nlines, char *map, size_t len)
{
	char *nl;
	size_t i;
	size_t off=0;

	for (i=0; i<nlines; ++i) {
		offs[i] = off;
		nl = memchr(map+off, '\n', len-off);
		if (!nl) break;
		off = nl - map + 1;
	}

	offs[nlines] = len;
}

void
run(char *map, size_t len)
{
	size_t i;
	size_t nlines;
	size_t *offs;
	struct set_node *nodes;
	struct set s[1] = {{0}};

	nlines = wc(map, len);

	offs = calloc(nlines + 1, sizeof *offs);
	if (!offs) die("malloc failed");

	nodes = calloc(nlines, sizeof *nodes);
	if (!nodes) die("malloc failed");

	lb(offs, nlines, map, len);

	for (i=0; i<nlines; ++i) {
		nodes[i].key = map+offs[i];
		set_add(s, nodes+i, offs[i+1] - offs[i]);
	}

	for (i=0; i<nlines; ++i) {
		set_has(s, map+offs[i], offs[i+1] - offs[i]);
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
