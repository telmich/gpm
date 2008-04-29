
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

#include <unistd.h>             /* write */

#include "types.h"              /* Gpm_type */
#include "daemon.h"             /* daemon internals */

int R_ms3(Gpm_Event * state, int fd)
{

   int dx, dy;
   char buf[4] = { 0, 0, 0, 0 };

   buf[0] |= 0x40;

   if(state->buttons & GPM_B_LEFT)
      buf[0] |= 0x20;
   if(state->buttons & GPM_B_MIDDLE)
      buf[3] |= 0x10;
   if(state->buttons & GPM_B_RIGHT)
      buf[0] |= 0x10;
   if(state->buttons & GPM_B_UP)
      buf[3] |= 0x0f;
   if(state->buttons & GPM_B_DOWN)
      buf[3] |= 0x01;

   dx = limit_delta(state->dx, -128, 127);
   buf[1] = dx & ~0xC0;
   buf[0] |= (dx & 0xC0) >> 6;

   dy = limit_delta(state->dy, -128, 127);
   buf[2] = dy & ~0xC0;
   buf[0] |= (dy & 0xC0) >> 4;

   /*
    * wheel 
    */
   if(state->wdy > 0)
      buf[3] |= 0x0f;
   else if(state->wdy < 0)
      buf[3] |= 0x01;

   return write(fd, buf, 4);
}
