#!/bin/sh
set -e

gcc -I. gpm2hack.c -o gpm2hack
./gpm2hack
