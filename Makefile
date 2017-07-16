all: obj bin test

include build.mk
include conf.mk

-include $(DEP)

obj: $(OBJ)
bin: $(BIN)
test: $(TESTS) $(patsubst %.c, run-%, $(TESTS))

clean:
	@echo cleaning
	@find . -name '*.c.o' -delete
	@find . -name '*.d' -delete
	@rm -f $(BIN)

%.c.o: %.c
	@$(call compile,$@,$<)

$(BIN):
	@$(call link,$@,$<)

run-test-%: test-%
	@$(info TEST $@)
	@test-$*

check:
	@for test in test-*; do [ -x "$$test" ] && "$$test" && echo; done || true

.PHONY: clean obj bin test all init fini
