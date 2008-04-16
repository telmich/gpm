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


/* Support for DEC VSXXX-AA and VSXXX-GA serial mice used on   */
/* DECstation 5000/xxx, DEC 3000 AXP and VAXstation 4000       */
/* workstations                                                */
/* written 2001/07/11 by Karsten Merker (merker@linuxtag.org)  */
/* modified (completion of the protocol specification and      */
/* corresponding correction of the protocol identification     */
/* mask) 2001/07/12 by Maciej W. Rozycki (macro@ds2.pg.gda.pl) */

int M_vsxxx_aa(Gpm_Event *state, unsigned char *data)
{

/* The mouse protocol is as follows:
 * 4800 bits per second, 8 data bits, 1 stop bit, odd parity
 * 3 data bytes per data packet:
 *              7     6     5     4     3     2     1     0
 * First Byte:  1     0     0   SignX SignY  LMB   MMB   RMB
 * Second Byte  0     DX    DX    DX    DX    DX    DX    DX
 * Third Byte   0     DY    DY    DY    DY    DY    DY    DY
 *
 * SignX:     sign bit for X-movement
 * SignY:     sign bit for Y-movement
 * DX and DY: 7-bit-absolute values for delta-X and delta-Y, sign extensions
 *            are in SignX resp. SignY.
 *
 * There are a few commands the mouse accepts:
 * "D" selects the prompt mode,
 * "P" requests a mouse's position (also selects the prompt mode),
 * "R" selects the incremental stream mode,
 * "T" performs a self test and identification (power-up-like),
 * "Z" performs undocumented test functions (a byte follows).
 * Parity as well as bit #7 of commands are ignored by the mouse.
 *
 * 4 data bytes per self test packet (useful for hot-plug):
 *              7     6     5     4     3     2     1     0
 * First Byte:  1     0     1     0     R3    R2    R1    R0
 * Second Byte  0     M2    M1    M0    0     0     1     0
 * Third Byte   0     E6    E5    E4    E3    E2    E1    E0
 * Fourth Byte  0     0     0     0     0    LMB   MMB   RMB
 *
 * R3-R0:     revision,
 * M2-M0:     manufacturer location code,
 * E6-E0:     error code:
 *            0x00-0x1f: no error (fourth byte is button state),
 *            0x3d:      button error (fourth byte specifies which),
 *            else:      other error.
 *
 * The mouse powers up in the prompt mode but we use the stream mode.
 */

  state->buttons = data[0]&0x07;
  state->dx = (data[0]&0x10) ? data[1] : -data[1];
  state->dy = (data[0]&0x08) ? -data[2] : data[2];
  return 0;
}
