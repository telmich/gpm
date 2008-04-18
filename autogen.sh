#!/bin/sh -e

[ -d config ] || mkdir config
${ACLOCAL-aclocal} -I config
${LIBTOOLIZE-libtoolize} --force --copy
${AUTOHEADER-autoheader}
${AUTOMAKE-automake} --add-missing --copy 2> /dev/null || true
${AUTOCONF-autoconf}

