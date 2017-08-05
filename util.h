#ifndef _edna_util_
#define _edna_util_

#define arr_len(arr) (sizeof (arr) / sizeof *(arr))

static inline
unsigned long
umin(unsigned long a, unsigned long b)
{
	return a < b ? a : b;
}

int msleep(size_t);
int mk_pty(void);
int open_pty(int);
#endif /* _util_ */
