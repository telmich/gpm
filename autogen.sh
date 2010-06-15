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
echo "define([AC_PACKAGE_VERSION], [${version} ${date}])" > .gitversion.m4
echo "${version}" > .gitversion
echo "${date}"    > .releasedate

${ACLOCAL-aclocal} -I config
${LIBTOOLIZE-libtoolize} -n --install 2>/dev/null && LIBTOOL_FLAGS="--install" || LIBTOOL_FLAGS=""
${LIBTOOLIZE-libtoolize} --force --copy ${LIBTOOL_FLAGS}
${AUTOHEADER-autoheader}
${AUTOCONF-autoconf}
