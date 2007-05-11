#
# cinit
#
# Nico Schottelius
#
# Don't edit Makefiles, use conf/* for configuration.
#

#include Makefile.tests

# Build tools / locations
CC=../tmpbin/cc
CFLAGS=
LD=../tmpbin/ld
LDFLAGS=
STRIP=../tmpbin/strip
SBIN=../sbin

# directories and files
CONFIG_H=include/config.h

BIN=cinit

#
# All objects depend on headers. More or less. FIXME: use gcc -M
#
CINIT_HEADERS=$(shell cat include/listing)

# NEW
CINIT_OBJ=$(shell cat object_lists/cinit)

#
# Client modules
#
CLIENT=client/msg_svc_on_off.o client/msg_change_status.o client/run_svc.o \
       client/exec_svc.o client/respawn_svc.o client/run_run_svcs.o \
       client/connect_sock.o client/begin_msg.o client/sig_terminate.o

COMMUNICATION=comm/do_change_status.o comm/do_result.o comm/do_svc_name.o

OBJ=$(SERV) $(SERV_OS) $(CLIENT) $(BOTH) $(COMMUNICATION)

CSVC_OBJ=util/cservice.o generic/mini_printf.o util/msg_reboot.o \
         generic/usage.o \
         $(CLIENT) $(COMMUNICATION)

CCO_OBJ=util/ccontrol.o generic/mini_printf.o util/msg_reboot.o \
         generic/usage.o \
         $(CLIENT) $(COMMUNICATION)
#
# End user target
#
#all: cinit cservice ccontrol sizecheck docs
#all: $(BIN)
all: cinit


#
# Generic
#
$(CINIT_OBJ): $(CINIT_HEADERS)

%.o: %.c
	$(CC) -c -o $@ $<



#
# First do configuration, so we include the right variables
#
Makefile: .configured

.configured: ../conf/*
	@../bin/cinit.configure.os
	@../bin/cinit.configure.tools
	@../bin/cinit.configure.ipc
	@../bin/cinit.configure.mark

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
	rm -f ../tmpbin/*.configured
	rm -f $(BIN) $(CINIT_OBJ)
	rm -f test/*.o
