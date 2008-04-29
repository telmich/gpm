
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
#include "daemon.h"             /* win */
#include "mice.h"               /* realposy */

#define CAL_LIMIT_BRK 255

#define CAL_X_MIN 0x40
#define CAL_X_MAX 0x1340
#define CAL_X_SIZE (CAL_X_MAX - CAL_X_MIN)

#define CAL_Y_MIN 0x40
#define CAL_Y_MAX 0xF40
#define CAL_Y_SIZE (CAL_Y_MAX - CAL_Y_MIN)

int M_calus(Gpm_Event * state, unsigned char *data)
{
   int x, y;

   x = ((data[1] & 0x3F) << 7) | (data[2] & 0x7F);
   y = ((data[4] & 0x1F) << 7) | (data[5] & 0x7F);

   state->buttons = GPM_B_LEFT * ((data[0] >> 2) & 1)
      + GPM_B_MIDDLE * ((data[0] >> 3) & 1)
      + GPM_B_RIGHT * ((data[0] >> 4) & 1);

   state->dx = 0;
   state->dy = 0;

   state->x = x < CAL_X_MIN ? 0
      : x > CAL_X_MAX ? win.ws_col + 1
      : (long) (x - CAL_X_MIN) * (long) (win.ws_col - 1) / CAL_X_SIZE + 2;

   state->y = y < CAL_Y_MIN ? win.ws_row + 1
      : y > CAL_Y_MAX ? 0
      : (long) (CAL_Y_MAX - y) * (long) win.ws_row / CAL_Y_SIZE + 1;

   realposx = x < CAL_X_MIN ? 0
      : x > CAL_X_MAX ? 16384
      : (long) (x - CAL_X_MIN) * (long) (16382) / CAL_X_SIZE + 2;

   realposy = y < CAL_Y_MIN ? 16384
      : y > CAL_Y_MAX ? 0
      : (long) (CAL_Y_MAX - y) * (long) 16383 / CAL_Y_SIZE + 1;

   return 0;
}
