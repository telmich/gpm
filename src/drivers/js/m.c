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

#ifdef HAVE_LINUX_JOYSTICK_H
/* Joystick mouse emulation (David Given) */
int M_js(Gpm_Event *state,  unsigned char *data)
{
   struct JS_DATA_TYPE *jdata = (void*)data;
   static int centerx = 0;
   static int centery = 0;
   static int oldbuttons = 0;
   static int count = 0;
   int dx;
   int dy;

   count++;
   if (count < 200) {
      state->buttons = oldbuttons;
      state->dx = 0;
      state->dy = 0;
      return 0;
   }
   count = 0;

   if (centerx == 0) {
      centerx = jdata->x;
      centery = jdata->y;
   }

   state->buttons = ((jdata->buttons & 1) * GPM_B_LEFT) |
                    ((jdata->buttons & 2) * GPM_B_RIGHT);
   oldbuttons     = state->buttons;

   dx             = (jdata->x - centerx) >> 6;
   dy             = (jdata->y - centery) >> 6;

   if (dx > 0) state->dx =   dx * dx;
   else        state->dx = -(dx * dx);
   state->dx >>= 2;

   if (dy > 0) state->dy = dy * dy;
   else        state->dy = -(dy * dy);
   state->dy >>= 2;

   /* Prevent pointer drift. (PC joysticks are notoriously inaccurate.) */

   if ((state->dx >= -1) && (state->dx <= 1)) state->dx = 0;
   if ((state->dy >= -1) && (state->dy <= 1)) state->dy = 0;

   return 0;
}
#endif /* have joystick.h */
