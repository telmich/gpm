#
# cinit
#
# Nico Schottelius
#
# Don't edit Makefiles, use conf/* for configuration.
#

#include Makefile.tests

# Build tools / locations
CC=./tmp/cc
CFLAGS=
LD=./tmp/ld
LDFLAGS=
STRIP=./tmp/strip
SBIN=../sbin

# directories and files
CONFIG_H=include/config.h

BIN=cinit

#
# End user targets
#
all: $(PROGS)

#
# Programs
#

PROGS=out/gpm2-daemon

out/gpm2-daemon: $(GPM2_DAEMON_OBJECTS) $(GPM2_DAEMON_PROTOCOLS)
	$(LD) -o $@ $^




#
# All objects depend on headers.
#
GPM2_DAEMON_HEADERS=$(shell cat deps/gpm2-daemon-headers)


#
# Objects
#

GPM2_DAEMON_OBJECTS=$(shell cat deps/gpm2-daemon)
$(GPM2_DAEMON_OBJECTS): $(GPM2_DAEMON_HEADERS)

GPM2_DAEMON_PROTOCOLS=$(shell cat tmp/protocol-deps)

CLIENT=

COMMUNICATION=

OBJ=

#
# Generic
#

%.o: %.c .configured
	$(CC) -c -o $@ $<



#
# First do configuration, so we include the right variables
#
Makefile: .configured

.configured: conf/*
	@mkdir -p tmp
	@./scripts/generate_cbuiltconfig.sh conf

$(CINIT_OBJ): $(CONFIG_H)


$(CONFIG_H): ../conf/*
	../scripts/cinit.mkheader > $(CONFIG_H)

cservice: $(SBIN)/cservice

$(SBIN)/cservice util/cservice: $(SBIN) $(CSVC_OBJ)
	$(LD) $(LDFLAGS) $(CSVC_OBJ) -o $@
	$(STRIP) $@

ccontrol: $(SBIN)/ccontrol

$(SBIN)/ccontrol util/ccontrol: config.h $(SBIN) $(CCO_OBJ)
	$(LD) $(LDFLAGS) $(CCO_OBJ) -o $@
	$(STRIP) $@

install: install-dir cinit cservice ccontrol
	@echo '*** Installing cinit ***'
	./bin/cinit.install.binary

install-miniconf:
	./bin/cinit.install.miniconf

install-dir:
	./bin/cinit.install.dir


################################################################################
#
# Build targets
#
cinit: $(CINIT_OBJ)
	$(LD) $^ -o $@

uml: cinit
	../../vm/uml/uml-mount.sh
	cp cinit ../../vm/uml/root/sbin/cinit
	../../vm/uml/uml-umount.sh

umlstart: uml
	../../vm/uml/uml-start.sh

################################################################################
#
# Generic targets
#

.PHONY: dist
dist: distclean

.PHONY: distclean
distclean: clean
	rm -f os/current ipc/current .configured

.PHONY: clean
clean:
	rm -f tmp/*
	rm -f $(BIN) $(CINIT_OBJ) $(GPM2_DAEMON_OBJECTS)
	rm -f test/*.o
