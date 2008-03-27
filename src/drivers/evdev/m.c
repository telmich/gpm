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


#ifdef HAVE_LINUX_INPUT_H

int M_evdev (Gpm_Event * state, unsigned char *data)
{
   struct input_event thisevent;
   (void) memcpy (&thisevent, data, sizeof (struct input_event));
   if (thisevent.type == EV_REL) {
      if (thisevent.code == REL_X)
         state->dx = (signed char) thisevent.value;
      else if (thisevent.code == REL_Y)
         state->dy = (signed char) thisevent.value;
   } else if (thisevent.type == EV_KEY) {
      switch(thisevent.code) {
         case BTN_LEFT:    state->buttons ^= GPM_B_LEFT;    break;
         case BTN_MIDDLE:  state->buttons ^= GPM_B_MIDDLE;  break;
         case BTN_RIGHT:   state->buttons ^= GPM_B_RIGHT;   break;
         case BTN_SIDE:    state->buttons ^= GPM_B_MIDDLE;  break;
      }
   }
   return 0;
}

#endif
