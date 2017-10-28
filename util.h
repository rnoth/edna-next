#ifndef _edna_util_
#define _edna_util_

#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

#define argv(...) ((char *[]){__VA_ARGS__})
#define arr_len(arr) (sizeof (arr) / sizeof *(arr))
#define fozin(P, F) ((P) ? (P)->F : 0)
#define write_str(fd, str) write(fd, str, strlen(str))

#define repeat(NTIMES) for (size_t _i=0; _i<NTIMES; ++i)
#define iterate(VAR, NITER) for (size_t VAR=0; VAR<NITER; ++VAR)

#ifndef LONG_BIT
# define LONG_BIT sizeof (long) * 8
#endif

typedef unsigned long ulong;
typedef unsigned int uint;

static inline bool in_range(ulong b, ulong e, ulong n);
static inline void ulrotate(ulong *l, ulong *m, ulong *r);
static inline void ulshift(ulong *l, ulong *m, ulong r);
static inline void plshift(void *l, void *m, void *r);
static inline void *memswp(void *A, void *B, size_t n);
static inline size_t next_line(char *s, size_t n);
static inline void pswp(void *A, void *B);
static inline int ucmp(ulong a, ulong b);
static inline int uclz(ulong a);
static inline int ufls(ulong a);
static inline ulong umax(ulong a, ulong b);
static inline ulong umin(ulong a, ulong b);

bool  in_range(ulong a, ulong z, ulong n) { return z > n - a; }
void  ulshift(ulong *l, ulong *m, ulong r) {*l = *m, *m = r;}
int   ucmp(ulong a, ulong b) { return (a >= b) - (a <= b); }
int   uclz(ulong a) { return a ? __builtin_clzl(a) : LONG_BIT; }
int   ufls(ulong a) { return LONG_BIT - uclz(a); }
ulong umax(ulong a, ulong b) {return a > b ? a : b;}
ulong umin(ulong a, ulong b) {return a < b ? a : b;}

void
ulrotate(ulong *l, ulong *m, ulong *r)
{
	ulong t = *l;
	*l=*m, *m=*r, *r=t;
}

void
plshift(void *L, void *M, void *r)
{
	void **l=L, **m=M;
	*l=*m, *m=r;
}

void *
memswp(void *A, void *B, size_t n)
{
	char *a=A, *b=B, m;
	while (n --> 0) m=a[n], a[n]=b[n], b[n]=m;
	return a;
}

size_t
next_line(char *s, size_t n)
{
	char *l;
	return (l = memchr(s, '\n', n)) ? l - s + 1 : n;
}

void
pswp(void *A, void *B)
{
	void **a=A, **b=B, *p;
	p=*a, *a=*b, *b=p;
}

char *asprintf(char *fmt, ...) __attribute__((format (printf, 1, 2)));
char *vasprintf(char *fmt, va_list args);
int msleep(size_t);
int mk_pty(void);
int open_pty(int);

#endif
