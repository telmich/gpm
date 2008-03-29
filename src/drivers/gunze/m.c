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

#include <sys/time.h>      /* gettimeofday      */

#include "types.h"                  /* Gpm_type         */
#include "message.h"                /* gpm_report       */
#include "mice.h"                   /* REALPOS           */
#include "daemon.h"                 /* which_mouse       */


int gunze_avg = 9; /* the bigger the smoother */
int gunze_calib[4]; /* x0,y0 x1,y1 (measured at 1/8 and 7/8) */
int gunze_debounce = 100; /* milliseconds: ignore shorter taps */

int M_gunze(Gpm_Event *state,  unsigned char *data)
{
   /*
    * This generates button-1 events, by now.
    * Check README.gunze for additional information.
    */
   int x, y;
   static int avgx, avgy;          /* average over time, for smooth feeling */
   static int upx, upy;            /* keep track of last finger-up place */
   static int released = 0, dragging = 0;
   static struct timeval uptv, tv; /* time of last up, and down events */
   int timediff;
    
   #define REAL_TO_XCELL(x) (x * win.ws_col / 0x3FFF)
   #define REAL_TO_YCELL(y) (y * win.ws_row / 0x3FFF)
    
   #define GET_TIME(tv) (gettimeofday(&tv, (struct timezone *)NULL))
   #define DIF_TIME(t1,t2) ((t2.tv_sec -t1.tv_sec) *1000+ \
                           (t2.tv_usec-t1.tv_usec)/1000)

   if (data[0] == 'R') {
      /*
       * finger-up event: this is usually offset a few pixels,
       * so ignore this x and y values. And invalidate avg.
       */
      upx = avgx;
      upy = avgy;
      GET_TIME(uptv); /* ready for the next finger-down */
      state->buttons = 0;
      released=1;
      return 0;
   }

   if(sscanf(data+1, "%d,%d", &x, &y)!= 2) {
      gpm_report(GPM_PR_INFO,GPM_MESS_GUNZE_INV_PACK,data);
      return 10; /* eat one byte only, leave ten of them */
   }
    /*
     * First of all, calibrate decoded data;
     * the four numbers are 1/8X 1/8Y, 7/8X, 7/8Y
     */
   x = 128 + 768 * (x - gunze_calib[0])/(gunze_calib[2]-gunze_calib[0]);
   y = 128 + 768 * (y - gunze_calib[1])/(gunze_calib[3]-gunze_calib[1]);

   /* range is 0-1023, rescale it upwards (0-REALPOS_MAX) */
   x = x * REALPOS_MAX / 1023;
   y = REALPOS_MAX - y * REALPOS_MAX / 1023;

   if (x<0) x = 0; if (x > REALPOS_MAX) x = REALPOS_MAX;
   if (y<0) y = 0; if (y > REALPOS_MAX) y = REALPOS_MAX;

    /*
     * there are some issues wrt press events: since the device needs a
     * non-negligible pressure to detect stuff, it sometimes reports
     * quick up-down sequences, that actually are just the result of a
     * little bouncing of the finger. Therefore, we should discard any
     * release-press pair. This can be accomplished at press time.
     */

   if (released) { /* press event -- or just bounce*/
      GET_TIME(tv);
      timediff = DIF_TIME(uptv, tv);
      released = 0;
      if (timediff > gunze_debounce && timediff < (which_mouse->opt_time)) {
         /* count as button press placed at finger-up pixel */
         dragging = 1;
         state->buttons = GPM_B_LEFT;
         realposx = avgx = upx; state->x = REAL_TO_XCELL(realposx);
         realposy = avgy = upy; state->y = REAL_TO_YCELL(realposy);
         upx = (upx - x); /* upx-upy become offsets to use for this drag */
         upy = (upy - y);
         return 0;
      }
      if (timediff <= gunze_debounce) {
         /* just a bounce, invalidate offset, leave dragging alone */
         upx = upy = 0;
      } else {
         /* else, count as a new motion event, reset avg */
         dragging = 0;
         realposx = avgx = x;
         realposy = avgy = y;
      }
   }

   state->buttons = 0;
   if (dragging) { /* a drag event: use position relative to press */
      x += upx;
      y += upy;
      state->buttons = GPM_B_LEFT;
   }

   realposx = avgx = (gunze_avg * avgx + x)/(gunze_avg+1);
   state->x = REAL_TO_XCELL(realposx);
   realposy = avgy = (gunze_avg * avgy + y)/(gunze_avg+1);
   state->y = REAL_TO_YCELL(realposy);

   return 0;

   #undef REAL_TO_XCELL
   #undef REAL_TO_YCELL
   #undef GET_TIME
   #undef DIF_TIME
}
