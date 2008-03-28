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

#include <unistd.h>        /* read/write        */
#include "mice.h"          /* Gpm_type         */


/*
 * Sends the SEND_ID command to the ps2-type mouse.
 * Return one of GPM_AUX_ID_...
 */
int read_mouse_id(int fd)
{
   unsigned char c = GPM_AUX_SEND_ID;
   unsigned char id;

   write(fd, &c, 1);
   read(fd, &c, 1);
   if (c != GPM_AUX_ACK) {
      return(GPM_AUX_ID_ERROR);
   }
   read(fd, &id, 1);

   return(id);
}

