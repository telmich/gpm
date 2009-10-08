
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

#include <termios.h>            /* termios */

#include "types.h"              /* Gpm_type */
#include "mice.h"               /* option_modem_lines */

Gpm_Type *I_pnp(int fd, unsigned short flags, struct Gpm_Type *type, int argc,
                const char **argv)
{
   struct termios tty;

   /*
    * accept "-o dtr", "-o rts" and "-o both" 
    */
   if(option_modem_lines(fd, argc, argv))
      return NULL;

   /*
    * Just put the device to 1200 baud. Thanks to Francois Chastrette
    * for his great help and debugging with his own pnp device.
    */
   tcgetattr(fd, &tty);

   tty.c_iflag = IGNBRK | IGNPAR;
   tty.c_oflag = 0;
   tty.c_lflag = 0;
   tty.c_line = 0;
   tty.c_cc[VTIME] = 0;
   tty.c_cc[VMIN] = 1;
   tty.c_cflag = flags | B1200;
   tcsetattr(fd, TCSAFLUSH, &tty);      /* set parameters */

   /*
    * Don't read the silly initialization string. I don't want to see
    * the vendor name: it is only propaganda, with no information.
    */

   return type;
}
