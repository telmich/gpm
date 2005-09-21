# Makefile for a basic kernel module 

CC=gcc 

#MODCFLAGS := -Wall -DMODULE -D__KERNEL__ -DLINUX
MODCFLAGS := -D__KERNEL__ -I/usr/src/linux-2.3.99-pre6/include -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer -fno-strict-aliasing -pipe -march=i586 -DMODULE -DMODVERSIONS -include /usr/src/linux-2.3.99-pre6/include/linux/modversions.h


touchscreen.o: touchscreen.c /usr/include/linux/version.h 
	$(CC) $(MODCFLAGS) -c touchscreen.c 

audio.o: audio.c /usr/include/linux/version.h 
	$(CC) $(MODCFLAGS) -c audio.c 
