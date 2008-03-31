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

int M_ms_plus(Gpm_Event *state, unsigned char *data)
{
   static unsigned char prev=0;

   state->buttons= ((data[0] & 0x20) >> 3) | ((data[0] & 0x10) >> 4);
   state->dx=      (signed char)(((data[0] & 0x03) << 6) | (data[1] & 0x3F));
   state->dy=      (signed char)(((data[0] & 0x0C) << 4) | (data[2] & 0x3F));

   /* Allow motion *and* button change (Michael Plass) */

   if((state->dx==0) &&(state->dy==0) && (state->buttons==(prev&~GPM_B_MIDDLE)))
      state->buttons = prev^GPM_B_MIDDLE;  /* no move or change: toggle middle */
   else
      state->buttons |= prev&GPM_B_MIDDLE;    /* change: preserve middle */

   prev=state->buttons;

   return 0;
}

