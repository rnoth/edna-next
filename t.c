#include <sys/mman.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <set.h>

#define die(bl) do { perror(bl); exit(1);} while (0)
int
main()
{
	struct set t[1] = {{0}};
	char *s;
	char *w;
	int d;
	off_t f;
	off_t i;
	ptrdiff_t n;

	d = open("words", O_RDONLY);
	if (d == -1) die("open failed");
	f = lseek(d, 0, SEEK_END);
	if (f == -1) die("lseek failed");
	w = mmap(0x0, f, PROT_READ, MAP_PRIVATE, d, 0);
	if (!w) die("mmap failed");

	i = 0;
	while (i < f) {
		n = (char *)memchr(w+i, '\n', f-i) - (w+i) + 1;
		s = set_alloc(n);
		memcpy(s, w+i, n);
		set_add(t, s);
		i += n;
	}

	i = 0;
	while (i < f) {
		n = (char *)memchr(w+i, '\n', f-i) - (w+i) + 1;
		set_has(t, w+i, n);
		i += n;
	}

	set_free(t);
	munmap(w, f);
	close(d);
}
