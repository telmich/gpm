#!/bin/sh
# I am lazy -- Nico
# 20080213
# Copying: GPLv3

ich="${0##*/}"
hier="${0%/*}"


"${hier}/build-from-git.sh" && sudo ./src/gpm

