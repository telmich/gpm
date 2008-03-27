/*
 * general purpose mouse (gpm)
 *
 * Copyright (c) 2008        Nico Schottelius <nico-gpm2008 at schottelius.org>
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
 ********/


#include <unistd.h>                 /* usleep, write */
#include <termios.h>                /* tcflush       */

#include "types.h"                  /* Gpm_type         */
#include "headers/message.h"        /* messaging in gpm */
#include "headers/daemon.h"         /* daemon internals */

/*
 * This works with Dexxa Optical Mouse, but because in X same initstring
 * is named ExplorerPS/2 so I named it in the same way.
 */
Gpm_Type *I_exps2(int fd, unsigned short flags, struct Gpm_Type *type, int argc, char **argv)
{
   static unsigned char s1[] = { 243, 200, 243, 200, 243, 80, };

   if (check_no_argv(argc, argv)) return NULL;

   write(fd, s1, sizeof (s1));
   usleep(30000);
   tcflush(fd, TCIFLUSH);
   return type;
}
