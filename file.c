#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <mem.h>

int
filemap_ctor(struct map *map, char *fn)
{
	off_t tmp;

	map->fd = open(fn, O_RDONLY);
	if (map->fd == -1) {
		return errno;
	}

	tmp = lseek(map->fd, 0, SEEK_END);
	if (tmp == -1) {
		close(map->fd);
		return errno;
	}
	map->length = tmp;
	map->map = mmap(0, map->length, PROT_READ, MAP_PRIVATE, map->fd, 0);
	if (map->map == MAP_FAILED) {
		close(map->fd);
		return errno;
	}

	return 0;
}

