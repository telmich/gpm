#!/bin/sh
# Nico Schottelius
# 2007-05-11
# First script to generate built environment from
# a standard cconfig directory for building.

[ "$#" -eq 1 ] || exit 23

set -e

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
   baseprog="$(basename "$tmp")"
   destprog="$tmpdir/$baseprog"

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

   echo "Creating $destprog: $prog $params"
   echo '#!/bin/sh' > "${destprog}"
   echo "\"${prog}\" $params \"\$@\"" >> "${destprog}"
   chmod 0700 "${destprog}"
done
