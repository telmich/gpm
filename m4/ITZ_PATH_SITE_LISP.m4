AC_DEFUN([ITZ_PATH_SITE_LISP],
[AC_CACHE_CHECK([where to install Emacs Lisp files],itz_cv_path_site_lisp,
[eval itz_cv_path_site_lisp=`${EMACS} -batch -l ${srcdir}/exec.el -exec "(mapcar 'print load-path)" 2>/dev/null |
sed -e '/^$/d' | sed -n -e 2p`
case x${itz_cv_path_site_lisp} in
x*site-lisp*) ;;
x*) itz_cv_path_site_lisp='${datadir}/emacs/site-lisp' ;;
esac])
])
