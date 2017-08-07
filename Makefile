all:: obj bin test

include build.mk
include conf.mk

-include $(DEP)

obj:: $(OBJ)
bin:: $(BIN)
test:: $(TESTS) $(patsubst %.c, .run-%, $(TESTS))

clean::
	@echo cleaning
	@rm -f *.c.o *.d .run-test-*

%.c.o: %.c
	@$(call compile,$@,$<)

$(BIN):
	@$(call link,$@,$<)

.run-test-%: test-%
	@$(info TEST $<)
	@$<
	@$(file > $@)

check:
	@for test in test-*; do [ -x "$$test" ] && "$$test" && echo; done || true

