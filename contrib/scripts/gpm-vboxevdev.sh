#!/bin/bash
DEV1=/dev/$(grep 'ImExPS/2' /sys/class/input/event*/device/name|cut -d/ -f4,5)
DEV2=/dev/$(grep 'VirtualBox mouse' /sys/class/input/event*/device/name|cut -d/ -f4,5)
GPM=../../src/gpm
#echo $DEV1 $DEV2 
cat <(cat $DEV1 & cat $DEV2) | $GPM -m - -t vboxevdev -D > /dev/null 2>/dev/null
