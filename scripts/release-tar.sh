#!/bin/sh
# Nico Schottelius
# 20080313

version=$1
hier=${0##*/}

echo $hier

[23:11] denkbrett:gpm% sha512sum gpm-1.20.3pre4.tar.bz2 > gpm-1.20.3pre4.tar.bz2.sha512sum
[23:12] denkbrett:gpm% scp gpm-1.20.3pre4.tar.bz2 gpm-1.20.3pre4.tar.bz2.sha512suM      
[23:12] denkbrett:gpm% chmod 0644 gpm-1.20.3pre4.tar.bz2 gpm-1.20.3pre4.tar.bz2.sha512sum
[23:12] denkbrett:gpm% scp gpm-1.20.3pre4.tar.bz2 gpm-1.20.3pre4.tar.bz2.sha512sum home.schottelius.org:www/org/schottelius/unix/www/gpm/archives


[ "$version" ] || exit 3

git archive --format=tar --prefix=gpm-${version}/ HEAD | \
   tee ../gpm-${version}.tar | bzip2 -9 > ../gpm-${version}.tar.bz2
