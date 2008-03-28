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


/* Summagraphics/Genius/Acecad tablet support absolute mode*/
/* Summagraphics MM-Series format*/
/* (Frank Holtz) hof@bigfoot.de Tue Feb 23 21:04:09 MET 1999 */
int SUMMA_BORDER=100,
    summamaxx,
    summamaxy;
char summaid=-1;


int M_summa(Gpm_Event *state, unsigned char *data)
{
   int x, y;

   x = ((data[2]<<7) | data[1])-SUMMA_BORDER;
   if (x<0) x=0;
   if (x>summamaxx) x=summamaxx;
   state->x = (x * win.ws_col / summamaxx);
   realposx = (x * 16383 / summamaxx);

   y = ((data[4]<<7) | data[3])-SUMMA_BORDER;
   if (y<0) y=0; if (y>summamaxy) y=summamaxy;
   state->y = 1 + y * (win.ws_row-1)/summamaxy;
   realposy = y * 16383 / summamaxy;

   state->buttons=
    !!(data[0]&1) * GPM_B_LEFT +
    !!(data[0]&2) * GPM_B_RIGHT +
    !!(data[0]&4) * GPM_B_MIDDLE;

   return 0;
}

