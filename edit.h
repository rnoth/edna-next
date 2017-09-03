#ifndef _edna_edit_
#define _edna_edit_

#include <mem.h>

int edit_append(struct map *edit, char *buffer, size_t length);
int edit_ctor(struct map *edit);
int edit_dtor(struct map *edit);
int map_ctor(struct map *map, char *fn);

#endif
