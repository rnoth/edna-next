#include <stdio.h>
#include <string.h>
#include <unit.h>
#include <util.h>
#include <set.c>

static void test_alloc(struct set *);
static void test_compare(void);
static void test_dict(struct set *);
static void test_free(struct set *);
static void test_index(void);
static void test_n_strings(char**);
//static void test_remove();
//static void test_remove_dict(struct set *);
static void test_tag(void);
static void test_two_strings(void);

struct unit_test tests[] = {
	{.msg = "should be able to allocate & free sets",
	 .fun = unit_list(test_alloc),
	 .ctx = (struct set[]){0}},
	{.msg = "should get sensible results from bit indexing functions",
	 .fun = unit_list(test_index),},
	{.msg = "should find that identical strings are identical",
	 .fun = unit_list(test_compare)},
	{.msg = "should be able to add strings",
	 .fun = unit_list(test_two_strings)},
	{.msg = "should have working tag functions",
	 .fun = unit_list(test_tag)},
	{.msg = "should be able to add multiple strings",
	 .fun = unit_list(test_n_strings),
	 .ctx = (char*[]){"one", "two", "three", "four",0}},
	{.msg = "should be able to add a whole dictionary",
	 .fun = unit_list(test_dict, test_free),
	 .ctx = (struct set[]){0}},
	#if 0
	{.msg = "should be able to remove strings",
	 .fun = unit_list(test_remove),},
	{.msg = "should be able to remove a lot of strings",
	 .fun = unit_list(test_dict, test_remove_dict, test_free),
	 .ctx = (struct set[]){0}},
	#endif
};

void
test_alloc(struct set *t)
{
	struct node *nod;
	int *p;

	ok(set_ctor(&p));
	ok(nod = node_of(p));

	ok((int*)nod->obj == p);
	ok(!nod->chld[0]);
	ok(!nod->chld[1]);

	free(nod);
}

void
test_compare()
{
	struct node *lef;
	struct node *rit;
	int *p;

	ok(set_ctor(&p));
	*p = 56;
	lef = node_of(p);
	ok(set_ctor(&p));
	*p = 56;
	rit = node_of(p);

	ok(nodes_are_matches(lef, rit));

	try(free(lef));
	try(free(rit));
}

void
test_dict(struct set *t)
{
	char b[256];
	char *s;
	FILE *f;

	ok(f = fopen("words", "r"));

	while (fgets(b, 256, f)) {
		b[strlen(b)-1] = 0;
		ok(s = set_str(b));
		try(set_add(t, s));
	}

	rewind(f);

	while (fgets(b, 256, f)) {
		b[strlen(b)-1] = 0;
		okf(set_has(t, b, strlen(b) + 1),
		            "assertion false: "
		            "set_has(t, \"%s\", strlen(\"%s\"))",
		            b, b);
	}

	fclose(f);
}

void
test_free(struct set *t)
{
	try(set_free(t));
}

void
test_index()
{
	expect(0, (bit_index_bytes((uint8_t[10]){0}, 10, 65)));
	expect(1, (bit_index_bytes((uint8_t[]){255}, 1, 5)));
	expect(1, (bit_index_bytes((uint8_t[]){128}, 1, 0)));
	expect(1, (bit_index_bytes((uint8_t[20]){[15]=16}, 20, 123)));
}

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

void
test_tag()
{
	struct node *node;
	uintptr_t back;
	uintptr_t tag;
	int *p;

	ok(set_ctor(&p));
	try(node = node_of(p));

	ok(tag = tag_node(node));
	ok(node == node_from_tag(tag));

	ok(tag = tag_leaf(node));
	ok(node == node_from_tag(tag));

	ok(back = tag_back(tag));
	ok(tag == tag_from_back(back));

	free(node);
}

void
test_two_strings()
{
	char *strs[3] = {"hello", "goodbye", 0x0};
	struct node *nod;
	struct set set[1] = {{0}};
	char *s;
	int i;

	for (i=0; strs[i]; ++i) {
		ok(s = set_alloc(strlen(strs[i]) + 1));
		strcpy(s, strs[i]);
		try(set_add(set, s));
		ok(set->root);
	}

	nod = node_from_tag(set->root);
	expect(4, nod->crit);
	nod = node_from_tag(nod->chld[0]);

	ok(set_has(set, strs[0], strlen(strs[0]) + 1));
	ok(set_has(set, strs[1], strlen(strs[1]) + 1));

	try(set_free(set));
}

void
test_n_strings(char **strs)
{
	struct set set[1] = {{0}};
	size_t i;
	char *s;

	
	for (i=0; strs[i]; ++i) {
		ok(s = set_alloc(strlen(strs[i]) + 1));
		strcpy(s, strs[i]);
		try(set_add(set, s));
	}

	for (i=0; strs[i]; ++i) {
		ok(set_has(set, strs[i], strlen(strs[i]) + 1));
	}

	set_free(set);
}

int
main(int argc, char **argv)
{
	unit_parse_argv(argv);
	return unit_run_tests(tests, arr_len(tests));
}
