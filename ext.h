#ifndef _edna_ext_
#define _edna_ext_

#include <stddef.h>
#include <stdint.h>

struct ext {
	uintptr_t root;
	size_t sum;
};

struct ext_node {
	uintptr_t chld[2];
	size_t sum;
	size_t ext;
};

#endif
