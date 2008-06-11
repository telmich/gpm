dnl $Id: aclocal.m4,v 1.2 2002/05/28 19:13:50 nico Exp $
AC_DEFUN([ITZ_SYS_ELF],
[AC_CACHE_CHECK([whether system is ELF],itz_cv_sys_elf,
[AC_EGREP_CPP(win,
[#ifdef __ELF__
win
#else
lose
#endif
],[itz_cv_sys_elf=yes],[itz_cv_sys_elf=no])])
if test ${itz_cv_sys_elf} = yes && test x${ac_cv_prog_gcc} = xyes ; then
        PICFLAGS="-DPIC -fPIC"
        SOLDFLAGS="-fPIC -shared -Wl,-soname,"
else
        PICFLAGS=
        SOLDFLAGS=
fi
])
AC_DEFUN([ITZ_PATH_SITE_LISP],
[AC_CACHE_CHECK([where to install Emacs Lisp files],itz_cv_path_site_lisp,
[eval itz_cv_path_site_lisp=`${EMACS} -batch -l ${srcdir}/exec.el -exec "(mapcar 'print load-path)" 2>/dev/null |
sed -e '/^$/d' | sed -n -e 2p`
case x${itz_cv_path_site_lisp} in
x*site-lisp*) ;;
x*) itz_cv_path_site_lisp='${datadir}/emacs/site-lisp' ;;
esac])
])
AC_DEFUN([ITZ_CHECK_TYPE],
[AC_CACHE_CHECK([for $1],itz_cv_type_$1,
AC_TRY_COMPILE([
#include <$2>
],[
$1 dummy;
return 0;
],[itz_cv_type_$1=yes],[itz_cv_type_$1=no]))
])

#
# Fix autoconf: They don't allow shell variables in AC_INIT
# So we have to define them via m4 to be accepted....
#
# What I simply wanted todo is 
#
#     AC_INIT([gpm],["$revgit"],[http://unix.schottelius.org/gpm/])
#
# What I have to do is seen below
#

#revgit="`cat $srcdir/.gitversion`"
#releasedate="`cat $srcdir/.releasedate`"
#m4_define([AC_PACKAGE_VERSION], [$revgit])
#m4_define([AC_PACKAGE_VERSION], include(.gitversion))
#AC_DEFINE_UNQUOTED([GPM_RELEASE_GIT],["$revgit"],[git short revision])
#AC_DEFINE_UNQUOTED([GPM_RELEASE_DATE],["$releasedate"],[release date])

#m4_define([AC_PACKAGE_VERSION], [fooooo])
m4_include(.gitversion)
