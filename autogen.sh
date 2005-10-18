#!/bin/sh

echo "Running autotools ..."

aclocal -I m4 || exit 1
autoheader || exit 1
autoconf || exit 1

echo "Done!  Now just run ./configure"
