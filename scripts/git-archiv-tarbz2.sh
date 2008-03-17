#!/bin/sh
# 
# 2008      Nico Schottelius (nico-git-dev at schottelius.org)
# 
#
# This file is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This file is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with ccollect. If not, see <http://www.gnu.org/licenses/>.
#
# Written on: 20080314
#

version="$1"
me=${0##*/}

if [ ! "$version" ]; then
   echo "${me}: Version"
   exit 1
fi

if [ ! -d .git ]; then
   echo "There is no .git in here."
   exit 2
fi

pwd=$(pwd)
name=${pwd##*/}

git archive --format=tar --prefix=${name}-${version}/ HEAD | \
   tee ../${name}-${version}.tar | bzip2 -9 > ../${name}-${version}.tar.bz2
