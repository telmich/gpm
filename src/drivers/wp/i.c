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

#include "types.h"                  /* Gpm_type         */


/*  Genius Wizardpad tablet  --  Matt Kimball (mkimball@xmission.com)  */
Gpm_Type *I_wp(int fd, unsigned short flags,
            struct Gpm_Type *type, int argc, char **argv)
{
   struct termios tty;
   char tablet_info[256];
   int count, pos, size;

   /* Set speed to 9600bps (copied from I_summa, above :) */
   tcgetattr(fd, &tty);
   tty.c_iflag = IGNBRK | IGNPAR;
   tty.c_oflag = 0;
   tty.c_lflag = 0;
   tty.c_line = 0;
   tty.c_cc[VTIME] = 0;
   tty.c_cc[VMIN] = 1;
   tty.c_cflag = B9600|CS8|CREAD|CLOCAL|HUPCL;
   tcsetattr(fd, TCSAFLUSH, &tty);

   /*  Reset the tablet (':') and put it in remote mode ('S') so that
     it isn't sending anything to us.  */
   write(fd, ":S", 2);
   tcsetattr(fd, TCSAFLUSH, &tty);

   /*  Query the model of the tablet  */
   write(fd, "T", 1);
   sleep(1);
   count = read(fd, tablet_info, 255);

   /*  The tablet information should start with "KW" followed by the rest of
     the model number.  If it isn't there, it probably isn't a WizardPad.  */
   if(count < 2) return NULL;
   if(tablet_info[0] != 'K' || tablet_info[1] != 'W') return NULL;

   /*  Now, we want the width and height of the tablet.  They should be 
     of the form "X###" and "Y###" where ### is the number of units of
     the tablet.  The model I've got is "X130" and "Y095", but I guess
     there might be other ones sometime.  */
   for(pos = 0; pos < count; pos++) {
      if(tablet_info[pos] == 'X' || tablet_info[pos] == 'Y') {
         if(pos + 3 < count) {
            size = (tablet_info[pos + 1] - '0') * 100 +
                   (tablet_info[pos + 2] - '0') * 10 +
                   (tablet_info[pos + 3] - '0');
            if(tablet_info[pos] == 'X') {
               wizardpad_width = size;
            } else {
               wizardpad_height = size;
            }
         }
      }
   }

   /*  Set the tablet to stream mode with 180 updates per sec.  ('O')  */
   write(fd, "O", 1);

   return type;
}

