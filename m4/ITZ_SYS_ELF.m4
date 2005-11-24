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
