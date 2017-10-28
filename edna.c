#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cmd.h>
#include <edna.h>
#include <file.h>
#include <ln.h>
#include <mem.h>
#include <txt.h>
#include <util.h>

struct action {
	struct action *chld;
	uintptr_t arg;
	struct piece *prev;
};

struct record {
	struct record *prev;
	struct action *acts;
};

static struct piece *act_free(struct action *act, struct piece *dead);
static struct piece *arrange_pieces(struct piece *chain);
static void free_pieces(struct piece *dead);
static struct piece *kill_piece(struct piece *dead, struct piece *new);
static void revert_insert(struct action *act);

struct piece *
act_free(struct action *cur, struct piece *dead)
{
	struct action *next;
	struct piece *result=0;

	while (cur) {
		next = cur->chld;

		result = untag(cur->arg);
		dead = kill_piece(dead, result);

		free(cur);

		cur = next;
	}

	return dead;
}

struct piece *
arrange_pieces(struct piece *chain)
{
	struct piece *ctx[2];
	struct piece *dead=0;

	ctx[0] = chain, ctx[1] = 0;
	do {
		text_step(ctx);
		dead = kill_piece(dead, ctx[1]);
	} while (text_next(ctx[0], ctx[1]));

	dead = kill_piece(dead, ctx[0]);

	return dead;
}

void
free_pieces(struct piece *dead)
{
	uintptr_t temp;

	while (dead) {
		temp = dead->link;
		free(dead);
		dead = untag(temp);
	}
}

struct piece *
kill_piece(struct piece *dead, struct piece *new)
{
	struct piece *head=dead;

	if (!new) return dead;

	if (!dead) {
		new->link = 0;
		return new;
	}

	if (tag0(dead) > tag0(new)) {
		new->link = tag0(dead);
		return new;
	}

	while (dead) {
		if (dead == new) return head;
		if (dead->link > tag0(new)) {
			new->link = dead->link;
			dead->link = tag0(new);
			return head;
		}

		if (!dead->link) break;
		dead = untag(dead->link);
	}

	new->link = 0;
	dead->link = tag0(new);

	return head;
}

struct piece *
rec_free(struct record *cur, struct piece *dead)
{
	struct record *next;
	struct action *act;

	while (cur) {
		act = cur->acts;
		dead = act_free(act, dead);
		next = cur->prev;
		free(cur);
		cur = next;
	}

	return dead;
}

void
revert_insert(struct action *act)
{
	struct piece *ctx[2];
	struct piece *next;
	struct piece *result;

	ctx[0] = untag(act->arg);
	ctx[1] = act->prev;

	next = text_next(ctx[0], ctx[1]);

	text_unlink(ctx[0], ctx[1]);
	text_unlink(next, ctx[0]);
	text_link(next, ctx[1]);

	result = ctx[0];
	ctx[0] = next;

	result->link = 0;

	text_merge(ctx);

	free(result);
}

void
edna_fini(struct edna *a)
{
	struct piece *p;

	p = arrange_pieces(a->txt);
	p = rec_free(a->hist, p);
	free_pieces(p);
	edit_dtor(a->edit);
	frag_free(a->ln);
}

int
edna_init(struct edna *edna)
{
	int err;

	*edna = (struct edna){0};

	edna->hist = calloc(1, sizeof *edna->hist);
	if (!edna->hist) return ENOMEM;

	edna->txt = text_ctor();
	if (!edna->txt) {
		free(edna->hist);
		return ENOMEM;
	}

	err = edit_ctor(edna->edit);
	if (err) {
		free(edna->hist);
		text_dtor(edna->txt);
		return err;
	}

	return 0;
}

int
edna_file_open(struct edna *a, size_t x, char *fn)
{
	struct piece *t[2];
	int e;

	e = filemap_ctor(a->file, fn);
	if (e) return e;

	text_start(t, a->txt);
	e = text_insert(t, x, a->file, 0, a->file->length);
	if (e) return e;

	e = ln_insert(&a->ln, x, a->file->map, a->file->length);
	if (e) __builtin_trap();

	a->dot[0] = a->file->length - a->ln->len;
	a->dot[1] = a->ln->len;

	return 0;
}

int
edna_text_delete(struct edna *a, size_t x, size_t n)
{
	struct action *c;
	struct piece *t[2];
	struct frag *p = frag_next(a->ln, 1);
	int e;

	c = calloc(1, sizeof *c);
	if (!c) return ENOMEM;

	text_start(t, a->txt);
	e = text_delete(t, x, n);
	if (e) {
		free(c);
		return e;
	}

	c->arg = tag0(t[0]);
	c->chld = a->hist->acts;

	ln_delete(&a->ln, x - a->dot[0], n);

	a->hist->acts = c;

	a->dot[1] = fozin(a->ln, len);
	if (!p) a->dot[0] -= a->dot[1];

	return 0;
}

int
edna_text_insert(struct edna *a, size_t x, char *s, size_t n)
{
	struct action *c;
	struct piece *t[2];
	struct frag *p;
	int e;

	text_start(t, a->txt);
	e = text_insert(t, x, a->edit, a->edit->offset, n);
	if (e) return e;

	c = calloc(1, sizeof *c);
	if (!c) return ENOMEM;

	c->arg = tag1(t[0]);
	c->prev = t[1];
	c->chld = a->hist->acts;

	e = edit_append(a->edit, s, n);
	if (e) {
		revert_insert(c);
		free(c);
		return e;
	}

	e = ln_insert(&a->ln, x - a->dot[0] - a->dot[1], s, n);
	if (e) {
		revert_insert(c);
		free(c);
		return e;
	}

	a->hist->acts = c;

	p = frag_next(a->ln, 0);
	a->dot[0] += fozin(p, len);
	a->dot[1] = a->ln->len;

	return 0;
}
