
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

#include "types.h"              /* Gpm_type */
#include "daemon.h"             /* which_mouse */

int M_ps2(Gpm_Event * state, unsigned char *data)
{
   static int tap_active = 0;   /* there exist glidepoint ps2 mice */

   state->buttons =
      !!(data[0] & 1) * GPM_B_LEFT +
      !!(data[0] & 2) * GPM_B_RIGHT + !!(data[0] & 4) * GPM_B_MIDDLE;

   if(data[0] == 0 && (which_mouse->opt_glidepoint_tap))        /* by default
                                                                 * this is
                                                                 * false */
      state->buttons = tap_active = (which_mouse->opt_glidepoint_tap);
   else if(tap_active) {
      if(data[0] == 8)
         state->buttons = tap_active = 0;
      else
         state->buttons = tap_active;
   }

   /*
    * Some PS/2 mice send reports with negative bit set in data[0] and zero for 
    * movement.  I think this is a bug in the mouse, but working around it only 
    * causes artifacts when the actual report is -256; they'll be treated as
    * zero. This should be rare if the mouse sampling rate is set to a
    * reasonable value; the default of 100 Hz is plenty. (Stephen Tell) 
    */
   if(data[1] != 0)
      state->dx = (data[0] & 0x10) ? data[1] - 256 : data[1];
   else
      state->dx = 0;
   if(data[2] != 0)
      state->dy = -((data[0] & 0x20) ? data[2] - 256 : data[2]);
   else
      state->dy = 0;
   return 0;
}
