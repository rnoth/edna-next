CC	?= cc
CFLAGS	+= -pipe -I. -D_POSIX_C_SOURCE=200809 -D_XOPEN_SOURCE=600 \
           -std=c99 -pedantic -Wall -Wextra \
           -fstrict-aliasing -fstrict-overflow -foptimize-sibling-calls \
           -fdata-sections -ffunction-sections -fno-exceptions \
           -fno-unwind-tables -fno-asynchronous-unwind-tables \
           -fno-stack-protector

LDFLAGS += -lc -Wl,--sort-section=alignment -Wl,--sort-common

SRC	:= $(wildcard *.c)
OBJ	:= $(SRC:.c=.c.o)
DEP	:= $(wildcard *.d)
TESTS	:= $(wildcard test-*.c)
BIN	:= edna $(TESTS:.c=) bench-set

edna: main.c.o
$(foreach test,$(TESTS),$(eval $(test:.c=): $(test:.c=.c.o)))
test-edna: edna

bench-set: bench-set.c.o

clean-test-edna: .run-test-edna
	@[ -f edna_test_file ] && rm edna_test_file || true
	@pgrep edna || true

all:: clean-test-edna

ifndef NDEBUG
CFLAGS	+= -O0 -ggdb3 -Werror
CFLAGS	+= -Wunreachable-code \
	   -Wno-missing-field-initializers -Wno-unused-parameter \
	   -Wno-sign-compare \
	   -Warray-bounds -Wno-missing-braces -Wno-parentheses \
	   -Wno-error=unused-function -Wno-error=unused-variable
else
LDFLAGS += -Wl,--gc-section
CFLAGS += -O3
endif
