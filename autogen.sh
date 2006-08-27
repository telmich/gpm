#!/bin/sh
# Nico Schottelius <nico-gpm /-at--/ schottelius.org>
# 2006-08-27
#

set -e
echo "Running autotools ..."
aclocal -I m4
autoheader
autoconf
echo "Done!  Now just run ./configure"
