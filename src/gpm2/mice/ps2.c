/*
 * gpm2 - mouse driver for the console
 *
 * Copyright (c) 2007         Nico Schottelius <nico-gpm2 () schottelius.org>
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
 *    handle standard ps2
 ********/

#include <unistd.h>        /* usleep()    */
#include <termios.h>       /* tcflush()   */

#include "gpm2-daemon.h"

/* protocol handler stolen from gpm1/mice.c */


/* standard ps2 */
int gpm2_open_ps2(int fd)
{
   static unsigned char s[] = { 246, 230, 244, 243, 100, 232, 3, };
   /* FIXME: check return */
   write(fd, s, sizeof (s));

   /* FIXME: time correct? */
   usleep (30000);
   tcflush (fd, TCIFLUSH);
   return 0;

}
