#!/bin/sh
# Nico Schottelius
# GPLv3

set -e

export PATH=$PATH:$(pwd -P)/protocols/ps2

mdev="$1"

: ${mdev:=/dev/input/mice}

make all
./gpm2d "$mdev" ps2

