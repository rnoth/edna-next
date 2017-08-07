#ifndef _edna_util_
#define _edna_util_

#define LONG_BIT (sizeof (unsigned long) * 8)
#define arr_len(arr) (sizeof (arr) / sizeof *(arr))

typedef unsigned long ulong;
typedef unsigned int uint;

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
umin(ulong a, ulong b)
{
	return a < b ? a : b;
}

char *asprintf(char *fmt, ...);
int msleep(size_t);
int mk_pty(void);
int open_pty(int);
#endif
