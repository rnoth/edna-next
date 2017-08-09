#ifndef _edna_exec_
#define _edna_exec_

struct parse;

int exec_ln(struct edna *edna, struct parse *parse);
int parse_ln(struct parse **dest, char *buffer, size_t length);

#endif
