#include <stdio.h>
#include <string.h>

#include <unit.h>
#include <util.h>
#include <set.c>

static void test_compare(void);
static void test_dict(struct set *);
static void test_index(void);
static void test_n_strings(char**);
//static void test_remove();
//static void test_remove_dict(struct set *);
static void test_two_strings(void);

struct unit_test tests[] = {
	{.msg = "should get sensible results from bit indexing functions",
	 .fun = unit_list(test_index),},
	{.msg = "should find that identical strings are identical",
	 .fun = unit_list(test_compare)},
	{.msg = "should be able to add strings",
	 .fun = unit_list(test_two_strings)},
	{.msg = "should be able to add multiple strings",
	 .fun = unit_list(test_n_strings),
	 .ctx = (char*[]){"one", "two", "three", "four",0}},
	{.msg = "should be able to add a whole dictionary",
	 .fun = unit_list(test_dict),
	 .ctx = (struct set[]){0}},
	#if 0
	{.msg = "should be able to remove strings",
	 .fun = unit_list(test_remove),},
	{.msg = "should be able to remove a lot of strings",
	 .fun = unit_list(test_dict, test_remove_dict),
	 .ctx = (struct set[]){0}},
	#endif
};

void
test_compare()
{
	ok(!~byte_diff((int[]){56}, (int[]){56}, sizeof (int)));
}

void
test_dict(struct set *t)
{
	struct set_node *s;
	char b[256];
	size_t n;
	FILE *f;

	ok(f = fopen("words", "r"));

	while (fgets(b, 256, f)) {
		n = strlen(b);
		b[n] = 0;
		ok(s = malloc(sizeof *s));
		ok(s->key = malloc(n));
		try(strcpy(s->key, b));
		try(set_add(t, s, n));
	}

	rewind(f);

	while (fgets(b, 256, f)) {
		n = strlen(b);
		b[n] = 0;
		okf(set_has(t, b, n),
		    "assertion false: "
		    "set_has(t, \"%s\", strlen(\"%s\") + 1)",
		    b, b);
	}

	fclose(f);
}

void
test_index()
{
	expect(0, (bit_index_bytes((uint8_t[10]){0}, 10, 65)));
	expect(1, (bit_index_bytes((uint8_t[]){255}, 1, 5)));
	expect(1, (bit_index_bytes((uint8_t[]){128}, 1, 0)));
	expect(1, (bit_index_bytes((uint8_t[20]){[15]=16}, 20, 123)));
}

#if 0

void
test_remove()
{
	char *strs[] = {"human", "error", 0};
	struct set set[1] = {{0}};
	int i;
	char *p;

	for (i=0; strs[i]; ++i) {
		ok(p = set_str(strs[i]));
		try(set_add(set, p));
	}

	for (i=0; strs[i]; ++i) {
		ok(set_has(set, strs[i], strlen(strs[i])+1))
		ok(set_rm(set, strs[i], strlen(strs[i])+1));
		ok(!set_rm(set, strs[i], strlen(strs[i])+1))
	}
}

void
test_remove_dict(struct set *t)
{
	char b[256];
	FILE *f;
	size_t n;
	long i;

	ok(f = fopen("words", "r"));

	while (fgets(b, 256, f) && fgets(b, 256, f)) {
		n = strlen(b) + 1;
		b[n-2] = 0;
		--n;
		ok(set_has(t,b,n));
		ok(set_rm(t,b,n));
		ok(!set_has(t,b,n));
		ok(!set_rm(t,b,n));
	}

	rewind(f);

	for (i=0; fgets(b, 256, f); ++i) {
		n = strlen(b) + 1;
		b[n---1] = 0;
		expect(i%2, set_has(t,b,n));
	}
}

#endif

void
test_two_strings()
{
	struct set_node goodbye_node[1];
	struct set_node hello_node[1];
	struct set set[1] = {{0}};
	char *goodbye = "goodbye";
	char *hello = "hello";
	size_t n;

	n = strlen(hello) + 1;
	hello_node->key = hello;
	try(set_add(set, hello_node, n));
	ok(set->root);

	n = strlen(goodbye) + 1;
	goodbye_node->key = goodbye;
	try(set_add(set, goodbye_node, n));
	expect(1, set->height);

	expect(4, goodbye_node->crit);

	ok(goodbye_node == node_from_tag(goodbye_node->chld[0]));
	ok(hello_node == node_from_tag(goodbye_node->chld[1]));

	ok(set_has(set, hello, strlen(hello) + 1));
	ok(set_has(set, goodbye, strlen(goodbye) + 1));
	ok(!set_has(set, "I should fail", strlen("I should fail")));
}

void
test_n_strings(char **strings)
{
	struct set set[1] = {{0}};
	struct set_node *node_array;
	size_t i;
	size_t n;

	for (i=0; strings[i]; ++i);

	ok(node_array = calloc(i + 1, sizeof *node_array));
	
	for (i=0; strings[i]; ++i) {
		n = strlen(strings[i]) + 1;
		node_array[i].key = strings[i];
		try(set_add(set, node_array+i, n));
	}

	for (i=0; strings[i]; ++i) {
		okf(set_has(set, strings[i], strlen(strings[i]) + 1),
		    "assertion false: "
		    "set_has(set, \"%s\", strlen(\"%s\") + 1)",
		    strings[i], strings[i]);
	}
}

int
main(int argc, char **argv)
{
	unit_parse_argv(argv);
	return unit_run_tests(tests, arr_len(tests));
}
