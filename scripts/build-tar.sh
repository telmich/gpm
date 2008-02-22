#!/bin/sh

version=$1

[ "$version" ] || exit 3

git archive --format=tar --prefix=gpm-${version}/ HEAD | \
   tee ../gpm-${version}.tar | bzip2 -9 > ../gpm-${version}.tar.bz2
