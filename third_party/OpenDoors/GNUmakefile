.DEFAULT_GOAL := all

GNUmakefile: ;

.PHONY: all static-lib shared-lib
all static-lib shared-lib:
	$(MAKE) -f src/GNUmakefile $@

%: FORCE
	$(MAKE) -f src/GNUmakefile $@

.PHONY: FORCE
FORCE:
