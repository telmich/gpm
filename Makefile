SRC=gpm2d.c init.c mouse_add.c mouse_init.c mouse_start.c mouse_list.c gpm2d_exit.c
SRC+=gpm2d_inputloop.c

CC=gcc
CFLAGS=-Wall -Werror

PROTOS=ps2

all: gpm2d protos

gpm2d: $(SRC) Makefile
	$(CC) $(CFLAGS) -g $(SRC) -I. -o $@

protos:
	for proto in $(PROTOS); do (cd protocols/$$proto && make all); done

clean:
	rm -f gpm2d
	for proto in $(PROTOS); do (cd protocols/$$proto && make clean); done
