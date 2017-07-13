all: init obj bin tests fini

include conf.mk
include build.mk

-include $(DEP)

obj: $(OBJ)
bin: $(BIN)
test: $(TESTS)

init: queue.d

fini: cleanup-queue

cleanup-queue:
	@rm queue.d

queue.d:
	@[ -e queue.d ] || printf '' > queue.d

clean:
	@echo cleaning
	@find . -name '*.c.o' -delete
	@find . -name '*.d' -delete
	@rm -f $(BIN)

%.c.o: %.c
	@$(info CC $@)
	@$(call makedeps,$*.c.d,$<)
	@$(call compile,$@,$<)
	@$(call add-syms,$*.c.d,$@)

tests: queue.d
	@$(eval $(foreach test, $(test-list), \
	          $(info EXEC $(test))$(shell $(test) > /dev/tty)))
	@printf '' > queue.d

check:
	@for test in test-*; do [ -x "$$test" ] && "$$test" && echo; done || true

$(BIN):
	@$(info LD $@)
	@$(call link,$@,$^)
	@$(call write-deps,$@.d,$@)
	@$(if $(filter $@.c,$(TESTS)),$(call enque,$@))

.PHONY: clean obj bin test all init fini
