
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
#include "daemon.h"             /* limit_delta */

/* ps2 */
int R_ps2(Gpm_Event * state, int fd)
{
   signed char buffer[3];

   buffer[0] = ((state->buttons & GPM_B_LEFT) > 0) * 1 +
      ((state->buttons & GPM_B_RIGHT) > 0) * 2 +
      ((state->buttons & GPM_B_MIDDLE) > 0) * 4;
   buffer[0] |= 8 + ((state->dx < 0) ? 0x10 : 0) + ((state->dy > 0) ? 0x20 : 0);
   buffer[1] = limit_delta(state->dx, -128, 127);
   buffer[2] = limit_delta(-state->dy, -128, 127);
   return write(fd, buffer, 3);
}
