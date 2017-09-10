#ifndef _edna_util_
#define _edna_util_

#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

#define arr_len(arr) (sizeof (arr) / sizeof *(arr))
#define write_str(fd, str) write(fd, str, strlen(str))

#ifndef LONG_BIT
# define LONG_BIT sizeof (long) * 8
#endif

typedef unsigned long ulong;
typedef unsigned int uint;

static inline
bool
in_range(size_t beg, size_t ext, size_t arg)
{
	return beg <= arg && beg + ext > arg;
}

static inline
void
lrotate(ulong *lef, ulong *mid, ulong *rit)
{
	ulong tmp = *lef;
	*lef = *mid;
	*mid = *rit;
	*rit = tmp;
}

static inline
size_t
next_line(char *buffer, size_t length)
{
	char *nl;

	nl = memchr(buffer, '\n', length);
	if (!nl) return length;
	return nl - buffer + 1;
}

static inline
void
ptr_swap(void *va, void *vb)
{
	void **a=va, **b=vb;
	void *tmp;

	tmp = *a;
	*a = *b;
	*b = tmp;
}

static inline
int
ucmp(ulong a, ulong b)
{
	return (a >= b) - (a <= b);
}

static inline
ulong
uclz(ulong a)
{
	if (!a) return LONG_BIT;
	return __builtin_clzl(a);
}

static inline
ulong
ufls(ulong a)
{
	return LONG_BIT - uclz(a);
}

static inline
ulong
umax(ulong a, ulong b)
{
	return a > b ? a : b;
}

static inline
ulong
umin(ulong a, ulong b)
{
	return a < b ? a : b;
}

char *asprintf(char *fmt, ...);
char *vasprintf(char *fmt, va_list args);
int msleep(size_t);
int mk_pty(void);
int open_pty(int);

#endif
