#ifndef _edna_edit_
#define _edna_edit_

struct map;

int edit_append(struct map *edit, char *buffer, size_t length);
int edit_ctor(struct map *edit);
int edit_dtor(struct map *edit);
int edit_expand(struct map *edit);

struct map {
	size_t offset;
	size_t length;
	char *map;
	int fd;
};

#endif
