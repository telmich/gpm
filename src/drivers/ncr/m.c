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


/* ncr pen support (Marc Meis) */

#define NCR_LEFT_X     40
#define NCR_RIGHT_X    2000

#define NCR_BOTTOM_Y   25
#define NCR_TOP_Y      1490

#define NCR_DELTA_X    (NCR_RIGHT_X - NCR_LEFT_X)
#define NCR_DELTA_Y    (NCR_TOP_Y - NCR_BOTTOM_Y)

static int M_ncr(Gpm_Event *state,  unsigned char *data)
{
   int x,y;

   state->buttons= (data[0]&1)*GPM_B_LEFT +
                !!(data[0]&2)*GPM_B_RIGHT;

   state->dx = (signed char)data[1]; /* currently unused */
   state->dy = (signed char)data[2];

   x = ((int)data[3] << 8) + (int)data[4];
   y = ((int)data[5] << 8) + (int)data[6];

   /* these formulaes may look curious, but this is the way it works!!! */

   state->x = x < NCR_LEFT_X
             ? 0
             : x > NCR_RIGHT_X
               ? win.ws_col+1
               : (long)(x-NCR_LEFT_X) * (long)(win.ws_col-1) / NCR_DELTA_X+2;

   state->y = y < NCR_BOTTOM_Y
             ? win.ws_row + 1
             : y > NCR_TOP_Y
          ? 0
          : (long)(NCR_TOP_Y-y) * (long)win.ws_row / NCR_DELTA_Y + 1;

   realposx = x < NCR_LEFT_X
             ? 0
             : x > NCR_RIGHT_X
               ? 16384
               : (long)(x-NCR_LEFT_X) * (long)(16382) / NCR_DELTA_X+2;

   realposy = y < NCR_BOTTOM_Y
             ? 16384
             : y > NCR_TOP_Y
          ? 0
          : (long)(NCR_TOP_Y-y) * (long)16383 / NCR_DELTA_Y + 1;

   return 0;
}

