#!/bin/sh
set -e

export PATH=$PATH:$(pwd -P)/protocols/ps2

mdev="$1"; shift

make gpm2d
./gpm2d "$mdev" ps2

