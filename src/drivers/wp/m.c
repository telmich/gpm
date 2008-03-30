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
#include "mice.h"                   /* REALPOS           */


/*  Genius Wizardpad tablet  --  Matt Kimball (mkimball@xmission.com)  */
int wizardpad_width = -1;
int wizardpad_height = -1;
int M_wp(Gpm_Event *state,  unsigned char *data)
{
   int x, y, pressure;

   x = ((data[4] & 0x1f) << 12) | ((data[3] & 0x3f) << 6) | (data[2] & 0x3f);
   state->x = x * win.ws_col / (wizardpad_width * 40);
   realposx = x * 16383 / (wizardpad_width * 40);

   y = ((data[7] & 0x1f) << 12) | ((data[6] & 0x3f) << 6) | (data[5] & 0x3f);
   state->y = win.ws_row - y * win.ws_row / (wizardpad_height * 40) - 1;
   realposy = 16383 - y * 16383 / (wizardpad_height * 40) - 1;  

   pressure = ((data[9] & 0x0f) << 4) | (data[8] & 0x0f);

   state->buttons=
      (pressure >= 0x20) * GPM_B_LEFT +
      !!(data[1] & 0x02) * GPM_B_RIGHT +
      /* the 0x08 bit seems to catch either of the extra buttons... */
      !!(data[1] & 0x08) * GPM_B_MIDDLE;

   return 0;
}
