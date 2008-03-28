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

#include <unistd.h>                 /* usleep            */
#include <termios.h>                /* tcflush           */

#include "mice.h"                   /* ‘GPM_AUX_ACK’     */

/**
 * Writes the given data to the ps2-type mouse.
 * Checks for an ACK from each byte.
 * 
 * Returns 0 if OK, or >0 if 1 or more errors occurred.
 */
int write_to_mouse(int fd, unsigned char *data, size_t len)
{
   int i;
   int error = 0;
   for (i = 0; i < len; i++) {
      unsigned char c;
      write(fd, &data[i], 1);
      read(fd, &c, 1);
      if (c != GPM_AUX_ACK) error++;
   }

   /* flush any left-over input */
   usleep (30000);
   tcflush (fd, TCIFLUSH);
   return(error);
}

