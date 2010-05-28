SRC=gpm2d.c init.c mouse_add.c mouse_init.c mouse_start.c mouse_list.c gpm2d_exit.c

CC=gcc

gpm2d: $(SRC) Makefile
	$(CC) -g $(SRC) -I. -o $@

clean:
	rm -f gpm2d
