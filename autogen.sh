#!/bin/sh -e

set -x

[ -d config ] || mkdir config


#
# Generate version in m4 format, because the following two in acinclude.m4
# do not work:
# 
# revgit="`cat $srcdir/.gitversion`"
# m4_define([AC_PACKAGE_VERSION], [$revgit])
# 
# and
# m4_define([AC_PACKAGE_VERSION], include(.gitversion))
#
version="$(git-describe)"
date="$(date +%Y%m%d\ %T\ %z)"
#echo "m4_define([AC_PACKAGE_VERSION], [${version}])" > .gitversion
echo "define([AC_PACKAGE_VERSION], [${version} ${date}])" > .gitversion

#date +%Y%m%d\ %T\ %z > .releasedate
${ACLOCAL-aclocal} -I config
${LIBTOOLIZE-libtoolize} --force --copy
${AUTOHEADER-autoheader}
${AUTOMAKE-automake} --add-missing --copy 2> /dev/null || true
${AUTOCONF-autoconf}

