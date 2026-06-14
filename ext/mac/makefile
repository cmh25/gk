export GKDIR ?= $(abspath ../..)
PKGS := $(sort $(patsubst %/makefile,%,$(wildcard */makefile)))

ifneq ($(shell uname -s),Darwin)
all test clean $(PKGS):
	@echo "ext/mac is macOS-only (host: $$(uname -s))"
else

all test clean:
	@rc=0; for p in $(PKGS); do $(MAKE) -C $$p $@ || rc=1; done; exit $$rc

$(PKGS):
	$(MAKE) -C $@

endif

.PHONY: all test clean $(PKGS)
