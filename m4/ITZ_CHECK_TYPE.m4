AC_DEFUN([ITZ_CHECK_TYPE],
[AC_CACHE_CHECK([for $1],itz_cv_type_$1,
AC_TRY_COMPILE([
#include <$2>
],[
$1 dummy;
return 0;
],[itz_cv_type_$1=yes],[itz_cv_type_$1=no]))
])
