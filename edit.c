#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <edit.h>
#include <mem.h>

static int edit_expand(struct map *edit, size_t min);

int
edit_append(struct map *edit, char *text, size_t length)
{
	size_t new;
	int err;

	new = edit->offset + length;
	if (new > edit->length) {
		err = edit_expand(edit, new);
		if (err) return err;
	}

	memcpy(edit->map+edit->offset, text, length);
	edit->offset = new;

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
edit_expand(struct map *edit, size_t min)
{
	size_t new;
	char *tmp;
	int err;

	for (new=edit->length; new<min; new*=2) ;

	err = msync(edit->map, edit->length, MS_SYNC);
	if (err) return errno;

	err = ftruncate(edit->fd, new);
	if (err) return errno;

	tmp = mmap(0, new, PROT_READ | PROT_WRITE,
	           MAP_SHARED, edit->fd, 0);
	if (tmp == MAP_FAILED) return errno;

	if (tmp != edit->map) {
		err = munmap(edit->map, edit->length);
		if (err) return errno;
	}

	edit->map = tmp;
	edit->length = new;

	return 0;
}
