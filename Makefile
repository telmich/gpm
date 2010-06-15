#			-*-makefile-*- (gpm/main)
#
# Copyright 1994,1997  rubini@linux.it
# Copyright 1997       dickey@clark.net
# Copyright (C) 1998 Ian Zimmerman <itz@rahul.net>
# Copyright (C) 2001 Nico Schottelius <nico@schottelius.org>
#

SHELL = /bin/sh
srcdir = /home/hamarti/libraries/gpm/gpm-1.20.6
top_srcdir = /home/hamarti/libraries/gpm/gpm-1.20.6
top_builddir = .

include Makefile.include

# allow CFLAGS to be overriden from make command line
# ideally one would never have to write this rule again, but the GNU
# makefile standards are at cross-purposes: CFLAGS is reserved for
# user-overridable flags, but it's also all the implicit rule looks at.
# missing ?

SUBDIRS = src doc contrib


### simple, but effective rules

all: do-all

dep:
	touch src/$(DEPFILE) # to prevent unecessary warnings
	make -C src dep

check: all

uninstall:	do-uninstall

clean: do-clean
	rm -f config.status.lineno


versionfiles =  .gitversion .gitversion.m4 .releasedate
dotfiles =  .gitignore .git/HEAD $(versionfiles)
distfiles = configure

# regen, if it changed; ignore if git is missing / fails
versioncheck:
	@v1="$$(git-describe 2>/dev/null)"; \
	if [ -z "$$v1" ]; then					\
		echo "No git available, not updating version"; 	\
		exit 0; 														\
	fi;											\
	v2="$$(cat .gitversion)"; 				\
	if [ "$$v1" != "$$v2" ]; then 		\
	echo "updating versionfiles"; 		\
	date="$$(date +%Y%m%d\ %T\ %z)"; 	\
	version="$$v1"; 							\
	echo "define([AC_PACKAGE_VERSION], [$${version} $${date}])" > .gitversion.m4; \
	echo "$${version}" > .gitversion; 	\
	echo "$${date}"    > .releasedate; 	\
	fi

$(versionfiles): .git/HEAD versioncheck

aclocal.m4: acinclude.m4
	$(MKDIR) config
	aclocal -I config

configure: configure.ac aclocal.m4 $(versionfiles)
	libtoolize --force --copy `libtoolize -n --install >/dev/null 2>&1 && echo --install`
	autoheader
	autoconf

config.status:	configure
	if [ -f config.status ]; then $(SHELL) ./config.status --recheck; \
	else $(SHELL) ./configure; fi

Makefile: config.status         $(srcdir)/Makefile.in Makefile.include
	./config.status
Makefile.include: config.status $(srcdir)/Makefile.include.in
	./config.status

### INSTALL

install:	check installdirs do-install

install-strip:
	$(MAKE) INSTALL_PROGRAM='$(INSTALL_PROGRAM) $(STRIP)' install

installdirs:
	$(MKDIR) $(libdir) $(bindir) $(sbindir) $(includedir) $(sysconfdir); \
	if test "x$(ELISP)" != "x" ; then \
		$(MKDIR) $(lispdir) ; \
	fi

### GENERIC

# do-all goes to all subdirectories and does all
do-%: dep
	@target=`echo $@ | $(SED) -e 's/^do-//'`; \
	for i in $(SUBDIRS) ; do \
		if test -f ./$$i/Makefile ; then \
			$(MAKE) -C ./$$i $${target} || exit 1 ;\
		else \
			true; \
		fi; \
       done


# Configure & unconfigure

# Maintainer portion, use at your own risk

barf:
	@echo 'This command is intended for maintainers to use; it'
	@echo 'deletes files that may need special tools to rebuild.'
	@echo 'If you want to continue, please press return.'
	@echo -n "Hit Ctrl+C to abort."
	@read somevar


# who / what does need tags
TAGS:	$(SRCS) $(HDRS) src/prog/gpm-root.y do-TAGS
	cd $(srcdir) && $(ETAGS) -o TAGS $(SRCS) $(HDRS) src/prog/gpm-root.y

### RELEASE STUFF
TARS  =../gpm-$(release).tar.gz
TARS +=../gpm-$(release).tar.bz2 ../gpm-$(release).tar.lzma
D_HOST=home.schottelius.org
D_BASE=www/unix.schottelius.org/www/gpm
D_DIR=$(D_BASE)/archives/
D_SOURCE=$(D_BASE)/browse_source/

M_HOST=arcana.linux.it
M_DIR=gpm/

tars: $(TARS)

# configure headers, produce new configure script
distconf: Makefile.in Makefile.include.in configure acinclude.m4 $(versionfiles)

../gpm-$(release).tar: $(srcdir) distclean distconf
	# no exclude possible of .git with pax it seems, so the following is not possible:
	#pax -w -x ustar -s '/^\./gpm-$(release)/' -w . -f $@
	pax -w -x ustar -s ';^;gpm-$(release)/;' -f $@ -w $(dotfiles) *

../gpm-$(release).tar.gz:  ../gpm-$(release).tar
	gzip -9 -c $< > $@

../gpm-$(release).tar.bz2:  ../gpm-$(release).tar
	bzip2 -9 -c $< > $@

../gpm-$(release).tar.lzma:  ../gpm-$(release).tar
	lzma -9 -c $< > $@

# 3. Put package together into .tar.gz and .tar.bz2
#dist: distclean distconf gpm-$(release).tar.bz2 gpm-$(release).tar.gz
dist: disttest distclean distconf nicotest $(TARS)
	chmod 0644 $(TARS)
	scp $(TARS) $(D_HOST):$(D_DIR)
	scp $(TARS) $(M_HOST):$(M_DIR)
	ssh "$(D_HOST)" "tar xvfz $(D_DIR)/gpm-$(release).tar.gz -C $(D_SOURCE)"
	ssh "$(D_HOST)" "find \"$(D_SOURCE)/\" -type f -exec chmod 0644 {} \\;"
	ssh "$(D_HOST)" "find \"$(D_SOURCE)/\" -type d -exec chmod 0755 {} \\;"


### TEST: on nicos machine: not to be used anywhere else currently
# create / update configure, delete other parts from earlier build
disttest: distconf clean
	./configure
	make all

TESTBUILDDIR=~/temp/gpm-build-test
nicotest: $(TARS)
	rm -rf $(TESTBUILDDIR)
	mkdir -p $(TESTBUILDDIR)
	cp $(TARS) $(TESTBUILDDIR)
	cd $(TESTBUILDDIR); tar xvfj gpm-$(release).tar.bz2;
	cd $(TESTBUILDDIR)/gpm-$(release); ./configure && make && ./src/gpm -v

### CLEANUP
distclean: clean do-clean do-distclean
	rm -f config.log config.status* config.cache Makefile Makefile.include
	rm -rf autom4te.cache 
	rm -f src/$(DEPFILE)

allclean: do-allclean distclean
	rm -f configure $(versionfiles) aclocal.m4
