/*
 *   Copyright (c) 2010 Nico Schottelius <nico-gpm2 () schottelius.org>
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
 *
 *
 * Create a small working hack to be expanded
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <gpm2.h>
#include "gpm2-daemon.h"

int main(int argc, char **argv)
{
   /* connect one mouse and print out data like display-coords.c */

   if(argc != 3) {
      printf("%s: <dev> ps2\n", argv[0]);
      _exit(1);
   }

   /* init global variables */
   init();

   /* add mouse: for testing, not in real code */
   mouse_add(argv[1], argv[2]);

   /* init clients */
//   gpm2_client_init();

   /* handle input */
//   gpm2_inputloop();

   /* close gpm2 */
   gpm2d_exit();
}
