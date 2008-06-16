
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
#include <unistd.h>             /* usleep, write */

int setspeed(int fd, int old, int new, int needtowrite, unsigned short flags)
{
   struct termios tty;

   char *c;

   tcgetattr(fd, &tty);

   tty.c_iflag = IGNBRK | IGNPAR;
   tty.c_oflag = 0;
   tty.c_lflag = 0;
   tty.c_line = 0;
   tty.c_cc[VTIME] = 0;
   tty.c_cc[VMIN] = 1;

   switch (old) {
      case 19200:
         tty.c_cflag = flags | B19200;
         break;
      case 9600:
         tty.c_cflag = flags | B9600;
         break;
      case 4800:
         tty.c_cflag = flags | B4800;
         break;
      case 2400:
         tty.c_cflag = flags | B2400;
         break;
      case 1200:
      default:
         tty.c_cflag = flags | B1200;
         break;
   }

   tcsetattr(fd, TCSAFLUSH, &tty);

   switch (new) {
      case 19200:
         c = "*r";
         tty.c_cflag = flags | B19200;
         break;
      case 9600:
         c = "*q";
         tty.c_cflag = flags | B9600;
         break;
      case 4800:
         c = "*p";
         tty.c_cflag = flags | B4800;
         break;
      case 2400:
         c = "*o";
         tty.c_cflag = flags | B2400;
         break;
      case 1200:
      default:
         c = "*n";
         tty.c_cflag = flags | B1200;
         break;
   }

   if(needtowrite)
      write(fd, c, 2);
   usleep(100000);
   tcsetattr(fd, TCSAFLUSH, &tty);
   return 0;
}
