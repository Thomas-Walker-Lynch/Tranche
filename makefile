# tranche/makefile
# sharing and installing are broken (this file comes from another project)

SHELL=/bin/bash

-include makefile-flags

.PHONY: all 
all: version

.PHONY: dep
dep:
	if [ -e $(DEPFILE) ]; then rm $(DEPFILE); fi
	$(MAKE) $@

.PHONY: lib
lib:
	cp $(SRCDIR)/tranche.lib.h include/tranche.h
	$(MAKE) $@

.PHONY: exec
exec:
	$(MAKE) $@

.PHONY: share
share:
	@echo "instead of share, the 'install' target will put the execs in tools/bin"

.PHONY: install
install:
	if [ -d $(EXECDIR) ]; then if [ ! -z "$(wildcard $(EXECDIR)/*)" ]; then cp $(EXECDIR)/* $(PROJECT_SUBU)/tool/bin; fi; fi

%::
	$(MAKE) $@




