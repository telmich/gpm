/*
 * general purpose mouse support for Linux
 *
 * *main.c*
 *
 * Copyright (c) 2002         Nico Schottelius <nico@schottelius.org>
 *
 * small main routine
 *
 * isn't that a nice clean function ?
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 ********/

#include "headers/daemon.h"
#include "headers/gpmInt.h"

int main(int argc, char **argv)
{
   startup(argc,argv);  /* setup configurations */
   old_main();          /* LATER: exit(daemon()); */
   return 0;            /* if we didn't exit before, just give back success */
}
