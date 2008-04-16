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

#include "daemon.h"         /* daemon internals */

/* 'Genitizer' (kw@dtek.chalmers.se 11/12/97) */
int M_geni(Gpm_Event *state,  unsigned char *data)
{
   /* this is a little confusing. If we use the stylus, we
    * have three buttons (tip, lower, upper), and if
    * we use the puck we have four buttons. (And the
    * protocol is a little mangled if several of the buttons
    * on the puck are pressed simultaneously.
    * I don't use the puck, hence I try to decode three buttons
    * only. tip = left, lower = middle, upper = right
    */
   state->buttons =
      (data[0] & 0x01)<<2 |
      (data[0] & 0x02) |
      (data[0] & 0x04)>>2;

   state->dx = ((data[1] & 0x3f) ) * ((data[0] & 0x10)?1:-1);
   state->dy = ((data[2] & 0x3f) ) * ((data[0] & 0x8)?-1:1);

   return 0;
}
