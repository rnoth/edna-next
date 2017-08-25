#ifndef _edna_mem_
#define _edna_mem_

struct map {
	size_t length;
	char *map;
	int fd;
};

int map_fd(struct map *map, int fd);
int memfd_create(char *name);

#endif
