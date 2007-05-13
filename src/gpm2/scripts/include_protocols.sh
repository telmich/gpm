#!/bin/sh
# Nico Schottelius
# 2007-05-13
# Create protocol related files

[ "$#" -eq 1 ] || exit 23

set -e

# args
confdir="$1"

# paths below the configuration directory
builtopts="built-options"
builtoptsdir="$confdir/$builtopts"

# paths directly in the srcdir
tmpdir="tmp"

#
# generate built programs
#

: > "${tmpdir}/protocols.h"

for proto in $(head -n1 ${builtoptsdir}/protocols); do
   echo "int gpm2_open_${proto}(int fd);" > "${tmpdir}/protocols.h"
done
