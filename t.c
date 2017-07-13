#include <stdio.h>
#include <set.c>

int
main()
{
	struct set t[1] = {{0}};
	char b[256];
	char *s;
	FILE *f;

	f = fopen("words", "r");

	while (fgets(b, 256, f)) {
		b[strlen(b)-1] = 0;
		s = set_alloc(strlen(b) + 1);
		strcpy(s, b);
		set_add(t, s);
	}

	rewind(f);

	while (fgets(b, 256, f)) {
		b[strlen(b)-1] = 0;
		set_has(t, b, strlen(b) + 1);
		set_has(t, b, strlen(b) + 1);
	}

	fclose(f);
	set_free(t);
}
