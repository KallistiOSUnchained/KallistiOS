# KallistiOS ##version##
#
# Root Makefile
# Copyright (C) 2003 Megan Potter
# Copyright (C) 2024 Falco Girgis
#

# Make sure things compile nice and cleanly. We don't necessarily want to push
# these flags out on to user code, but it's a good idea to keep them around for
# compiling all of KOS proper.
# I expect everyone to keep their code warning free. Don't make me add -Werror
# here too. ;-)
KOS_CFLAGS += -Wextra -Wno-deprecated

# Add stuff to SUBDIRS to auto-compile it with the big tree.
SUBDIRS = utils kernel addons # examples

# Detect a non-working or missing environ.sh file.
ifndef KOS_BASE
error:
	@echo You don\'t seem to have a working  environ.sh file. Please take a look at
	@echo doc/README for more info.
	@exit 0
endif

# Separate targets for kernel, utils, and addons
kernel: kernel_subdirs

utils: utils_subdirs

addons: kernel addons_subdirs

all: subdirs

clean: clean_subdirs

distclean: clean
	-rm -f lib/$(KOS_ARCH)/*
	-rm -f addons/lib/$(KOS_ARCH)/*

docs:
	doxygen $(KOS_BASE)/doc/Doxyfile

docs_clean:
	-rm -rf $(KOS_BASE)/doc/reference

docs_open: docs
	open $(KOS_BASE)/doc/reference/html/index.html

kos-ports_all:
	$(KOS_PORTS)/utils/build-all.sh

kos-ports_clean:
	$(KOS_PORTS)/utils/clean-all.sh

kos-ports_distclean: kos-ports_clean
	$(KOS_PORTS)/utils/uninstall-all.sh

all_auto_kos_base:
	$(MAKE) all KOS_BASE=$(CURDIR)

clean_auto_kos_base:
	$(MAKE) clean KOS_BASE=$(CURDIR)

# Subdirectory rules
subdirs:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir; \
	done

clean_subdirs:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

# Specific rules for kernel, utils, and addons
kernel_subdirs:
	$(MAKE) -C kernel

utils_subdirs:
	$(MAKE) -C utils

addons_subdirs:
	$(MAKE) -C addons

include $(KOS_BASE)/Makefile.rules
