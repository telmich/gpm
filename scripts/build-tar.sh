#!/bin/sh
# Nico Schottelius
# GPLv3
# 20080314

version=$1

[ "$version" ] || exit 3

if [ ! -d .git ]; then
   echo noooooo.
   exit 1
fi

pwd=$(pwd)
name=${pwd##*/}

git archive --format=tar --prefix=${name}-${version}/ HEAD | \
   tee ../${name}-${version}.tar | bzip2 -9 > ../${name}-${version}.tar.bz2
