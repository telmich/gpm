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
 *    read one mouse packet and return it
 ********/

#include <unistd.h>        /* read()      */
#include <stdio.h>         /* NULL        */
#include <stdlib.h>        /* malloc()    */
#include <errno.h>         /* errno       */

char *read_packet(int fd, int len)
{
   char *packet;
   
   errno = 0;  /* no library function ever sets errno to 0, so we do */

   packet = malloc(len);
   if(packet == NULL) return NULL;

   if(read(fd,&packet,len) == -1) {
      free(packet);
      return NULL;
   }

   return packet;
}
