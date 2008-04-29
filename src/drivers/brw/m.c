
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

#include "mice.h"               /* Gpm_type */
#include "message.h"            /* messaging */

/* M_brw is a variant of m$ 'Intellimouse' the middle button is different */
int M_brw(Gpm_Event * state, unsigned char *data)
{
   state->buttons = ((data[0] & 0x20) >> 3)     /* left */
      |((data[3] & 0x20) >> 4)  /* middle */
      |((data[0] & 0x10) >> 4); /* right */
   state->dx = (signed char) (((data[0] & 0x03) << 6) | (data[1] & 0x3F));
   state->dy = (signed char) (((data[0] & 0x0C) << 4) | (data[2] & 0x3F));
   if(((data[0] & 0xC0) != 0x40) ||
      ((data[1] & 0xC0) != 0x00) ||
      ((data[2] & 0xC0) != 0x00) || ((data[3] & 0xC0) != 0x00)) {
      gpm_report(GPM_PR_DEBUG, GPM_MESS_SKIP_DATAP, data[0], data[1], data[2],
                 data[3]);
      return -1;
   }
   /*
    * wheel (dz) is (data[3] & 0x0f) 
    */
   /*
    * where is the side button? I can sort of detect it at 9600 baud 
    */
   /*
    * Note this mouse is very noisy 
    */

   return 0;
}
