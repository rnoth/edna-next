#ifndef _edna_ext_
#define _edna_ext_

#include <stddef.h>
#include <stdint.h>

struct ext_node {
	uintptr_t chld[2];
	size_t off;
	size_t ext;
};

struct ext {
	struct ext_node zero[1];
	uintptr_t root;
};

#endif
