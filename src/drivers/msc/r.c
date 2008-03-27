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

int R_msc(Gpm_Event *state, int fd)
{
   signed char buffer[5];
   int dx, dy;

   /* sluggish... */
   buffer[0]=(state->buttons ^ 0x07) | 0x80;
   dx = limit_delta(state->dx, -256, 254);
   buffer[3] =  state->dx - (buffer[1] = state->dx/2); /* Markus */
   dy = limit_delta(state->dy, -256, 254);
   buffer[4] = -state->dy - (buffer[2] = -state->dy/2);
   return write(fd,buffer,5);

}

