# 
# gpm - configure.ac
# based on configure.in, written jan/1997 byT.E.Dickey <dickey@clark.net>
# Nico Schottelius
#
# heavily cleaned up

############################# prework #################################
AC_INIT(gpm, 1.20.2 (git), gpm@lists.linux.it)
AC_PREREQ(2.59)
AC_CONFIG_SRCDIR([src/gpm.c])
AC_CONFIG_HEADER([src/headers/config.h])

############################# release version #################################
release="1.20.2-monotone"
release_date="Sometime 2004"
AC_DEFINE_UNQUOTED(GPM_RELEASE,"$release")
AC_DEFINE_UNQUOTED(GPM_RELEASE_DATE,"$release_date")

# These are chosen so that we can switch to the libtool scheme transparently.
abi=20
abi_age=19
abi_rev=0

abi_lev=`expr $abi - $abi_age`
abi_full=$abi_lev.$abi_age.$abi_rev

dnl Make srcdir an absolute path.
if test x$srcdir = x ; then
        srcdir = `pwd`
else
        srcdir=`cd $srcdir && pwd`
fi

# check for programs
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_YACC
AC_PROG_LN_S
AC_PATH_PROG(DVIPS,dvips,no)
AC_PATH_PROG(TEXI2HTML,texi2html,no)
AC_PATH_PROG(TEX,tex,no)
AC_PATH_PROG(TEXINDEX,texindex,no)
AC_PATH_PROG(MAKEINFO,makeinfo,no)
AC_PATH_PROG(DIFF,diff,no)
AC_PATH_PROG(SED,sed,no)
AC_PATH_PROG(emacs,emacs,no)

# emacs
if test ${ac_cv_path_emacs} = no ; then
        EMACS=:
        ELISP=
else
        EMACS=${ac_cv_path_emacs}
        ELISP="emacs/t-mouse.el emacs/t-mouse.elc"
fi

if test $EMACS != : ; then
        ITZ_PATH_SITE_LISP
        lispdir=${itz_cv_path_site_lisp}
else
        lispdir='${datadir}/emacs/site-lisp'
fi

# headers
AC_CHECK_HEADERS(linux/input.h linux/joystick.h linux/major.h linux/tty.h)
AC_CHECK_HEADERS(curses.h ncurses.h ncurses/curses.h sys/sysmacros.h syslog.h)
AC_CHECK_HEADERS([math.h], , [AC_MSG_ERROR([a mandatory header is missing])])

# curses
AC_ARG_WITH(curses,
[  --without-curses        disable curses support even if curses found])

CURSES_OBJS=

case $with_curses in
No|no|N|n) : ;;
        *) if   test ${ac_cv_header_ncurses_h} = yes || 
                test ${ac_cv_header_ncurses_curses_h} = yes ||
                test ${ac_cv_header_curses_h} = yes ; then
        CURSES_OBJS=lib/libcurses.o ; fi ;;
esac

EVDEV_SRCS=
if test ${ac_cv_header_linux_input_h} = yes ; then
        EVDEV_SRCS=evdev.c ;
        AC_CHECK_TYPE(struct input_absinfo,AC_DEFINE_UNQUOTED(HAVE_INPUT_ABSINFO, 1, [define if struct input_absinfo defined in linux/input.h]),,[#include <linux/input.h>])
fi

ITZ_CHECK_TYPE(__u32,linux/types.h)
if test ${itz_cv_type___u32} = yes || test ${ac_cv_type___u32} = yes ; then
    AC_DEFINE(HAVE___U32)
fi

ITZ_SYS_ELF
if test ${itz_cv_sys_elf} = yes && test x${ac_cv_prog_gcc} = xyes ; then
    SHLIB=lib/libgpm.so
else
    SHLIB=
fi

############################# functions #######################################
AC_CHECK_FUNCS(vsyslog syslog)
AC_FUNC_ALLOCA

case $with_curses in
No|no|N|n) SHARED_LIBS=-lc ;;
*)
    SAVELIBS=$LIBS
    LIBS=
    for i in tinfo ncurses termcap termlib; do
        if test x$LIBS = x; then
            AC_CHECK_LIB($i, tputs)
        else :; fi
    done
    TERMLIBS=$LIBS
    LIBS=
    for i in ncurses curses; do
        if test x$LIBS = x; then
            AC_CHECK_LIB($i, wgetch,,,$TERMLIBS)
        else :; fi
    done
    SHARED_LIBS="$LIBS $TERMLIBS -lc"
    LIBS=$SAVELIBS ;;
esac    

dnl ceil() may be an inline, so check with no libm first
AC_CACHE_CHECK([whether libm is needed for ceil], [gpm_cv_ceil_libm],
[ceil_found=no
for i in "" "-lm"; do
    SAVELIBS=$LIBS
    LIBS=$i
    AC_LINK_IFELSE([AC_LANG_PROGRAM(
[#ifdef HAVE_MATH_H
#include <math.h>
#endif], [ceil(0.0);])], [ceil_found=yes])
    LIBS=$SAVELIBS
    if test x$ceil_found = xyes; then
        break
    fi
done
if test x$ceil_found = xno; then
    gpm_cv_ceil_libm=missing
elif test x$i = x-lm; then
    gpm_cv_ceil_libm=yes
else
    gpm_cv_ceil_libm=no
fi])
case $gpm_cv_ceil_libm in
no)
    LIBM=
    ;;
yes)
    LIBM=-lm
    ;;
*)
    AC_MSG_ERROR([a mandatory function is missing])
    ;;
esac

GPMXTERM=
AC_SUBST(GPMXTERM)
AC_SUBST(release)
AC_SUBST(release_date)
AC_SUBST(abi_lev)
AC_SUBST(abi_full)
AC_SUBST(EMACS)
AC_SUBST(ELISP)
AC_SUBST(SHLIB)
AC_SUBST(PICFLAGS)
AC_SUBST(SOLDFLAGS)
AC_SUBST(CURSES_OBJS)
AC_SUBST(EVDEV_SRCS)
AC_SUBST(SHARED_LIBS)
AC_SUBST(LIBM)
AC_SUBST(lispdir)

CPPFLAGS='-I$(srcdir) $(DEFS) -include headers/config.h -Wall -DSYSCONFDIR="\"$(sysconfdir)\"" -DSBINDIR="\"$(sbindir)\""'
LDFLAGS='-L$(srcdir)'

AC_OUTPUT(Makefile.include 
            Makefile
            contrib/Makefile
            doc/Makefile
            doc/doc.gpm
            src/Makefile)

# old stuff
#AC_INIT(src/gpm.c)

