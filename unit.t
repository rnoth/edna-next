#ifdef _unit_main_
# error "multiple inclusion of template (this is probably an error)"
#else

# define _unit_main_

#ifndef UNIT_TESTS
#define UNIT_TESTS tests
#endif

#include <sysexits.h>

int
main(int argc, char **argv)
{
	int err;

	err = unit_parse_argv(argc, argv);
	if (err) return EX_USAGE;

	err = unit_run_tests(UNIT_TESTS, arr_len(UNIT_TESTS));
	if (err) return EX_SOFTWARE;

	return 0;
}

#endif
