#include <stdio.h>
#include <unit.h>
#include <util.h>
#include <set.c>

static void test_alloc(struct set *);
static void test_compare();
static void test_dict();
static void test_index();
static void test_n_strings();
static void test_tag();
static void test_two_strings();

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
	 .fun = unit_list(test_dict),},
};

void
test_alloc(struct set *t)
{
	struct node *nod;
	int *p;

	unit_ok(set_ctor(&p));
	unit_ok(nod = node_of(p));

	unit_ok((int*)nod->obj == p);
	unit_ok(nod->size == sizeof *p);
	unit_ok(!nod->chld[0]);
	unit_ok(!nod->chld[1]);

	free(nod);
}

void
test_compare()
{
	struct node *lef;
	struct node *rit;
	int *p;

	unit_ok(set_ctor(&p));
	*p = 56;
	lef = node_of(p);
	unit_ok(set_ctor(&p));
	*p = 56;
	rit = node_of(p);

	unit_ok(nodes_are_matches(lef, rit));

	unit_try(free(lef));
	unit_try(free(rit));
}

void
test_dict()
{
	struct set t[1] = {{0}};
	char b[256];
	char *s;
	FILE *f;

	unit_ok(f = fopen("words", "r"));

	while (fgets(b, 256, f)) {
		b[strlen(b)-1] = 0;
		unit_ok(s = set_alloc(strlen(b) + 1));
		strcpy(s, b);
		unit_try(set_add(t, s));
	}

	rewind(f);

	while (fgets(b, 256, f)) {
		b[strlen(b)-1] = 0;
		unit_ok_fmt(set_has(t, b, strlen(b) + 1),
		            "assertion false: "
		            "set_has(t, \"%s\", strlen(\"%s\"))",
		            b, b);
		unit_ok(set_has(t, b, strlen(b) + 1));
	}

	fclose(f);
	set_free(t);
}

void
test_index()
{
	unit_expect(0, (bit_index_bytes((uint8_t[10]){0}, 10, 65)));
	unit_expect(1, (bit_index_bytes((uint8_t[]){255}, 1, 5)));
	unit_expect(1, (bit_index_bytes((uint8_t[]){128}, 1, 0)));
	unit_expect(1, (bit_index_bytes((uint8_t[20]){[15]=16}, 20, 123)));
}

void
test_tag()
{
	struct node *node;
	uintptr_t back;
	uintptr_t tag;
	int *p;

	unit_ok(set_ctor(&p));
	unit_try(node = node_of(p));

	unit_ok(tag = tag_node(node));
	unit_ok(node == node_from_tag(tag));

	unit_ok(tag = tag_leaf(node));
	unit_ok(node == node_from_tag(tag));

	unit_ok(back = tag_back(tag));
	unit_ok(tag == tag_from_back(back));

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
		unit_ok(s = set_alloc(strlen(strs[i]) + 1));
		strcpy(s, strs[i]);
		unit_try(set_add(set, s));
		unit_ok(set->root);
	}

	nod = node_from_tag(set->root);
	unit_expect(4, nod->crit);
	nod = node_from_tag(nod->chld[0]);

	unit_ok(set_has(set, strs[0], strlen(strs[0]) + 1));
	unit_ok(set_has(set, strs[1], strlen(strs[1]) + 1));

	unit_try(set_free(set));
}

void
test_n_strings(char **strs)
{
	struct set set[1] = {{0}};
	size_t i;
	char *s;

	
	for (i=0; strs[i]; ++i) {
		unit_ok(s = set_alloc(strlen(strs[i]) + 1));
		strcpy(s, strs[i]);
		unit_try(set_add(set, s));
	}

	for (i=0; strs[i]; ++i) {
		unit_ok(set_has(set, strs[i], strlen(strs[i]) + 1));
	}

	set_free(set);
}

int
main(int argc, char **argv)
{
	return unit_run_tests(argv, tests, arr_len(tests));
}
