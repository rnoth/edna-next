#ifndef _edna_mem_
#define _edna_mem_

struct map;

int memfd_create(char *name);

struct map {
	size_t offset;
	size_t length;
	char *map;
	int fd;
};

#endif
