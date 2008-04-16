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
#include "daemon.h"                 /* which_mouse       */


int M_imps2(Gpm_Event *state,  unsigned char *data)
{

   static int tap_active=0;         /* there exist glidepoint ps2 mice */
   state->wdx  = state->wdy   = 0;  /* Clear them.. */
   state->dx   = state->dy    = state->wdx = state->wdy = 0;

   state->buttons= ((data[0] & 1) << 2)   /* left              */
      | ((data[0] & 6) >> 1);             /* middle and right  */

   if (data[0]==0 && (which_mouse->opt_glidepoint_tap)) // by default this is false
      state->buttons = tap_active = (which_mouse->opt_glidepoint_tap);
   else if (tap_active) {
      if (data[0]==8)
         state->buttons = tap_active = 0;
      else state->buttons = tap_active;
   }

   /* Standard movement.. */
   state->dx = (data[0] & 0x10) ? data[1] - 256 : data[1];
   state->dy = (data[0] & 0x20) ? -(data[2] - 256) : -data[2];

   /* The wheels.. */
   unsigned char wheel = data[3] & 0x0f;
   if (wheel > 0) {
     // use the event type GPM_MOVE rather than GPM_DOWN for wheel movement
     // to avoid single/double/triple click processing:
     switch (wheel) {
       /* rodney 13/mar/2008
        * The use of GPM_B_UP / GPM_B_DOWN is very unclear;
        *  only mouse type ms3 uses these
        * For this mouse, we only support the relative movement
        * i.e. no button is set (same as mouse movement), wdy changes +/-
        *  according to wheel movement (+ for rolling away from user)
        * wdx (horizontal scroll) is for a second wheel. They do exist! */
        case 0x0f: state->wdy = +1; break;
        case 0x01: state->wdy = -1; break;
        case 0x0e: state->wdx = +1; break;
        case 0x02: state->wdx = -1; break;
     }
   }

   return 0;

}

