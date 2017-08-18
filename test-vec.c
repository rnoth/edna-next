#include <unit.h>
#include <util.h>
#include <vec.h>

static void test_alloc(void);
static void test_clone(void);
static void test_concat(void);
static void test_copy(void);
static void test_delete(void);
static void test_elim(void);
static void test_insert(void);
static void test_free(void);
static void test_large_insert(void);
static void test_pop(void);
static void test_shift(void);
static void test_slice(void);
static void test_splice(void);
static void test_transfer(void);
static void test_truncat(void);

static struct unit_test tests[] = {
	{.msg="allocating & freeing a vector",
	 .fun=unit_list(test_alloc, test_free), },
	{.msg="adding some elements to a simple vector",
	 .fun=unit_list(test_alloc, test_insert, test_free), },
	{.msg="adding a large number of elements",
	 .fun=unit_list(test_large_insert, test_free), },
	{.msg="deleting elements from a vector",
	 .fun=unit_list(test_large_insert, test_delete, test_free), },
	{.msg="eliminating elements from a vector",
	 .fun=unit_list(test_large_insert, test_elim, test_free), },
	{.msg="duplicating a vector",
	 .fun=unit_list(test_large_insert, test_clone, test_free), },
	{.msg="truncating a vector",
	 .fun=unit_list(test_large_insert, test_truncat, test_free), },
	{.msg="concatenating a vector with an array",
	 .fun=unit_list(test_large_insert, test_concat, test_free), },
	{.msg="shifting a vector",
	 .fun=unit_list(test_large_insert, test_shift, test_free), },
	{.msg="copying vectors",
	 .fun=unit_list(test_large_insert, test_copy, test_free), },
	{.msg="slicing a vector",
	 .fun=unit_list(test_large_insert, test_slice, test_free), },
	{.msg="splicing a vector with an array",
	 .fun=unit_list(test_large_insert, test_splice, test_free), },
	{.msg="transfering elements to an array",
	 .fun=unit_list(test_large_insert, test_transfer, test_free), },
	{.msg="popping elements from a vector",
	 .fun=unit_list(test_large_insert, test_pop, test_free), },
};

static int *intvec;

void
test_free(void)
{
	if (intvec) {
		try(vec_free(intvec));
		intvec = 0x0;
	}

	if (fflush(stdout)) {
		perror("fflush");
		exit(1);
	}
}

void
test_alloc(void)
{
	ok(!vec_ctor(intvec));
	ok(vec_len(intvec) ==  0);

}

void
test_clone(void)
{
	int *cln = vec_clone(intvec);

	ok(!!cln);

	ok(vec_len(cln) == vec_len(intvec));
	ok(!memcmp(cln, intvec, vec_len(cln) * sizeof *cln));

	try(vec_free(cln));

}

void
test_concat(void)
{
#	define ARR_LEN 100
	int arr[ARR_LEN];
	int i;

	for (i = 1; i < 10 * 100; i *= 10) {
		arr[i/10] = i;
	}

	expect(0, vec_concat(&intvec, arr, ARR_LEN));
	ok(vec_len(intvec) == 1100);

	for (i = 1; i < 10 * 100; i *= 10) {
		expect(i, intvec[1000 + i/10]);
	}
#	undef ARR_LEN
}

void
test_copy(void)
{
	int *cpy;

	expect(0, vec_ctor(cpy));

	expect(0, vec_copy(&cpy, intvec));
	expect(1000, vec_len(cpy));

	ok(!memcmp(cpy, intvec, 1000 * sizeof *cpy));

	expect(0, vec_copy(&intvec, cpy));
	expect(1000, vec_len(intvec));

	ok(!memcmp(cpy, intvec, 1000 * sizeof *cpy));
	try(vec_free(cpy));

}

void
test_delete(void)
{
	for (int i = 501; i < 600; ++i) {
		try(vec_delete(&intvec, 500));
		expect(900 + 600 - i, vec_len(intvec));
		ok(intvec[500] == i);
	}
}

void
test_elim(void)
{
	try(vec_elim(&intvec, 200, 400));
	expect(600, vec_len(intvec));

	for (int i = 0; i < 200; ++i) {
		ok(intvec[i] == i);
	}

	for (int i = 200; i < 600; ++i) {
		ok(intvec[i] == i + 400);
	}

}

void
test_insert(void)
{
	expect(0, vec_append(&intvec, (int[]){5}));
	ok(vec_len(intvec) == 1);
	ok(intvec[0] == 5);

	expect(0, vec_insert(&intvec, (int[]){3}, 0));
	ok(vec_len(intvec) == 2);
	ok(intvec[0] == 3);
	ok(intvec[1] == 5);

	expect(0, vec_insert(&intvec, (int[]){312345}, 1));
	ok(vec_len(intvec) == 3);
	ok(intvec[0] == 3);
	ok(intvec[1] == 312345);
	ok(intvec[2] == 5);

	expect(0, vec_insert(&intvec, (int[]){15557145}, 1));
	ok(vec_len(intvec) == 4);
	ok(intvec[0] == 3);
	ok(intvec[1] == 15557145);
	ok(intvec[2] == 312345);
	ok(intvec[3] == 5);

}

void
test_large_insert(void)
{
	ok(!vec_ctor(intvec));

	for (int i = 0; i < 1000; ++i) {
		expect(0, vec_append(&intvec, &i));
		ok(intvec[i] == i);
		ok(vec_len(intvec) == (size_t)i + 1);
	}

}

void
test_pop(void)
{
	int i;
	vec_pop(&i, &intvec);
	expect(999, vec_len(intvec));
	expect(999, i);
}

void
test_shift(void)
{
	int i;

	try(vec_shift(&intvec, 250));
	for (i = 0; i < 750; ++i) expect(i + 250, intvec[i]);

}

void
test_slice(void)
{
	int i;

	try(vec_slice(&intvec, 300, 600));

	expect(600, vec_len(intvec));
	for (i = 0; i < 600; ++i) {
		expect(i + 300, intvec[i]);
	}

}

void
test_splice(void)
{
#	define ARR_LEN 100
	int i;
	int arr[ARR_LEN];

	for (i = 0; i < 100; ++i) arr[i] = intvec[i + 600];

	expect(0, vec_splice(&intvec, 400, arr, ARR_LEN));
	expect(1100, vec_len(intvec));

	for (i = 0; i < 400; ++i) expect(i, intvec[i]);
	ok(!memcmp(intvec + 400, arr, ARR_LEN));
	for (i = 0; i < 600; ++i) expect(i + 400, intvec[i + 500]);
#	undef ARR_LEn
}

void
test_transfer(void)
{
#	define ARR_LEN 100
	int arr[ARR_LEN];
	int i;

	for (i = 0; i < 100; ++i) arr[i] = intvec[i + 600];

	expect(0, vec_transfer(&intvec, arr, ARR_LEN));
	expect(100, vec_len(intvec));

	ok(!memcmp(intvec, arr, 100 * sizeof *intvec));
#	undef ARR_LEN
}

void
test_truncat(void)
{
	try(vec_truncat(&intvec, 500));
	ok(vec_len(intvec) == 500);

	try(vec_truncat(&intvec, 0));
	ok(vec_len(intvec) == 0);
}

int
main(int argc, char **argv)
{
	unit_parse_argv(argv);
	unit_run_tests(tests, arr_len(tests));
}
