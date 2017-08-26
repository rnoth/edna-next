#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <edit.h>
#include <mem.h>

int
edit_append(struct map *edit, char *text, size_t length)
{
	int err;

	if (edit->offset + length > edit->length) {
		err = edit_expand(edit);
		if (err) return err;
	}

	memcpy(edit->map+edit->offset, text, length);
	edit->offset += length;

	return 0;
}

int
edit_dtor(struct map *edit)
{
	munmap(edit->map, edit->length);
	close(edit->fd);

	return 0;
}

int
edit_ctor(struct map *edit)
{
	edit->fd = memfd_create("edna-edit");
	if (edit->fd == -1) return errno;

	edit->length = sysconf(_SC_PAGESIZE);
	ftruncate(edit->fd, edit->length);
	edit->map = mmap(0, edit->length, PROT_READ | PROT_WRITE,
	                   MAP_SHARED, edit->fd, 0);
	if (edit->map == MAP_FAILED) {
		close(edit->fd);
		return errno;
	}

	return 0;
}

int
edit_expand(struct map *edit)
{
	char *tmp;
	int err;

	err = msync(edit->map, edit->length, MS_SYNC);
	if (err) return errno;

	tmp = mmap(0, edit->length*2, PROT_READ | PROT_WRITE,
	           MAP_SHARED, edit->fd, 0);
	if (tmp == MAP_FAILED) return errno;

	err = munmap(edit->map, edit->length);
	if (err) return errno;

	edit->map = tmp;
	edit->length *= 2;

	return 0;
}
