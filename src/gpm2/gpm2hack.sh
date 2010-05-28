#!/bin/sh
set -e

export PATH=$PATH:/home/users/nico/gpm2/protocols/ps2

which gpm2-ps2
make gpm2d
./gpm2d

