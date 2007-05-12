#!/bin/sh
# Nico Schottelius
# 2007-05-11
# First script to generate built environment from
# a standard cconfig directory for building.

[ "$#" -eq 1 ] || exit 23

# args
confdir="$1"

# paths below the configuration directory
programs="programs"
progdir="$confdir/$programs"

# paths directly in the srcdir
tmpdir="tmp"

#
# generate built programs
#

for tmp in "${progdir}"/*; do
   prog=""
   params=""
   baseprog="$(basename "$tmp")

   # ignore *.params, those are parameters, not programs
   if [ "${tmp%.params}" != "${tmp}" ]; then
      continue
   fi

   # check for params
   pfile="${tmp}.params"
   if [ -f "$pfile" ]; then
      params="$(head -n1 "$pfile")"
   fi

   prog=$(head -n1 "$tmp")

   echo "Creating $tmp: $prog $params"
done
