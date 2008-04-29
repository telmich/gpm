
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

#include <sys/time.h>           /* gettimeofday */

#include "types.h"              /* Gpm_type */
#include "mice.h"               /* mice */
#include "daemon.h"             /* which_mouse */

int M_mtouch(Gpm_Event * state, unsigned char *data)
{
   /*
    * This is a simple decoder for the MicroTouch touch screen
    * devices. It uses the "tablet" format and only generates button-1
    * events. Check README.microtouch for additional information.
    */
   int x, y;
   static int avgx = -1, avgy;  /* average over time, for smooth feeling */
   static int upx, upy;         /* keep track of last finger-up place */
   static struct timeval uptv, tv;      /* time of last up, and down events */

#define REAL_TO_XCELL(x) (x * win.ws_col / 0x3FFF)
#define REAL_TO_YCELL(y) (y * win.ws_row / 0x3FFF)

#define GET_TIME(tv) (gettimeofday(&tv, (struct timezone *)NULL))
#define DIF_TIME(t1,t2) ((t2.tv_sec -t1.tv_sec) *1000+ \
                           (t2.tv_usec-t1.tv_usec)/1000)

   if(!(data[0] & 0x40)) {
      /*
       * finger-up event: this is usually offset a few pixels,
       * so ignore this x and y values. And invalidate avg.
       */
      upx = avgx;
      upy = avgy;
      GET_TIME(uptv);           /* ready for the next finger-down */
      tv.tv_sec = 0;
      state->buttons = 0;
      avgx = -1;                /* invalidate avg */
      return 0;
   }

   x = data[1] | (data[2] << 7);
   y = 0x3FFF - (data[3] | (data[4] << 7));

   if(avgx < 0) {               /* press event */
      GET_TIME(tv);
      if(DIF_TIME(uptv, tv) < (which_mouse->opt_time)) {
         /*
          * count as button press placed at finger-up pixel 
          */
         state->buttons = GPM_B_LEFT;
         realposx = avgx = upx;
         state->x = REAL_TO_XCELL(realposx);
         realposy = avgy = upy;
         state->y = REAL_TO_YCELL(realposy);
         upx = (upx - x);       /* upx and upy become offsets to use for this
                                 * drag */
         upy = (upy - y);
         return 0;
      }
      /*
       * else, count as a new motion event 
       */
      tv.tv_sec = 0;            /* invalidate */
      realposx = avgx = x;
      state->x = REAL_TO_XCELL(realposx);
      realposy = avgy = y;
      state->y = REAL_TO_YCELL(realposy);
   }

   state->buttons = 0;
   if(tv.tv_sec) {              /* a drag event: use position relative to press 
                                 */
      x += upx;
      y += upy;
      state->buttons = GPM_B_LEFT;
   }

   realposx = avgx = (9 * avgx + x) / 10;
   state->x = REAL_TO_XCELL(realposx);
   realposy = avgy = (9 * avgy + y) / 10;
   state->y = REAL_TO_YCELL(realposy);

   return 0;

#undef REAL_TO_XCELL
#undef REAL_TO_YCELL
#undef GET_TIME
#undef DIF_TIME
}
