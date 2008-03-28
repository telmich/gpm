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

#include <unistd.h>                 /* usleep, write     */

#include "types.h"                  /* Gpm_type         */
#include "mice.h"                   /* check_no_argv     */
#include "message.h"                /* gpm_report        */



Gpm_Type *I_netmouse(int fd, unsigned short flags, struct Gpm_Type *type, int argc, char **argv)
{
   unsigned char magic[6] = { 0xe8, 0x03, 0xe6, 0xe6, 0xe6, 0xe9 };
   int i;

   if (check_no_argv(argc, argv)) return NULL;
   for (i=0; i<6; i++) {
      unsigned char c = 0;
      write( fd, magic+i, 1 );
      read( fd, &c, 1 );
      if (c != 0xfa) {
         gpm_report(GPM_PR_ERR,GPM_MESS_NETM_NO_ACK,c);
         return NULL;
      }
   }
   {
      unsigned char rep[3] = { 0, 0, 0 };
      read( fd, rep, 1 );
      read( fd, rep+1, 1 );
      read( fd, rep+2, 1 );
      if (rep[0] || (rep[1] != 0x33) || (rep[2] != 0x55)) {
         gpm_report(GPM_PR_ERR,GPM_MESS_NETM_INV_MAGIC, rep[0], rep[1], rep[2]);
         return NULL;
      }
   }
   return type;
}

