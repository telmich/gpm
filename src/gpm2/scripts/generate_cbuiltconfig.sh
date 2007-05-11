#!/bin/sh
# Nico Schottelius
# 2007-05-11
# First script to generate built environment from
# a standard cconfig directory for building.

[ "$#" -eq 1 ] || exit 23

# args
confdir="$1"

# paths below the configuration directory
programs="programs"

# paths directly in the srcdir
tmp="tmp"

#
# generate built programs
#


