#ifndef _edna_stk_
#define _edna_stk_

struct frame {
	uint8_t op;
	void *ret;
	void **arg;
	struct frame *up;
};

struct exec {
	struct frame *stk;
	struct queue *que;
};

struct queue {
	struct frame fra;
	struct queue *next;
};

#endif
