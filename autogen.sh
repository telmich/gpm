#!/bin/sh

[ -d config ] || mkdir config

version=$(git describe 2>/dev/null)

# No git? use changelog information
if [ -z "$version" ]; then
    version=$(grep '^[[:digit:]]' doc/changelog | head -n1 | cut -d: -f1)
    date=$(grep '^[[:digit:]]' doc/changelog | head -n1 | cut -d: -f2)
else
    date=$(git log -1 --pretty="format:%ai" "$version")
fi

cat << eof > configure.ac
AC_INIT([gpm],[$version],[http://www.nico.schottelius.org/software/gpm/])

releasedate="$date"
release="$version"
eof

cat configure.ac.footer >> configure.ac

${ACLOCAL-aclocal} -I config
${LIBTOOLIZE-libtoolize} -n --install 2>/dev/null && LIBTOOL_FLAGS="--install" || LIBTOOL_FLAGS=""
${LIBTOOLIZE-libtoolize} --force --copy ${LIBTOOL_FLAGS}
${AUTOHEADER-autoheader}
${AUTOCONF-autoconf}
