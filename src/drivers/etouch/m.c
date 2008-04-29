
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
#include "etouch.h"             /* etouch specs */
#include "mice.h"               /* REALPOS */
#include "daemon.h"             /* which_mouse */

int elo_click_ontouch = 0;      /* the bigger the smoother */

extern int gunze_calib[4];      /* FIXME: do not depend on other drivers! */

int M_etouch(Gpm_Event * state, unsigned char *data)
{                               /* 
                                 * This is a simple decoder for the EloTouch touch screen devices.
                                 * ELO format SmartSet UTsXXYYZZc 9600,N,8,1
                                 * c=checksum = 0xAA+'T'+'U'+s+X+X+Y+Y+Z+Z (XXmax=YYmax=0x0FFF=4095)
                                 * s=status   bit 0=init touch  1=stream touch  2=release
                                 */
#define ELO_CLICK_ONTOUCH       /* ifdef then ButtonPress on first Touch else
                                 * first Move then Touch */
   int x, y;
   static int avgx = -1, avgy;  /* average over time, for smooth feeling */
   static int upx, upy;         /* keep track of last finger-up place */
   static struct timeval uptv, tv;      /* time of last up, and down events */

#define REAL_TO_XCELL(x) (x * win.ws_col / 0x3FFF)
#define REAL_TO_YCELL(y) (y * win.ws_row / 0x3FFF)

#define GET_TIME(tv) (gettimeofday(&tv, (struct timezone *)NULL))
#define DIF_TIME(t1,t2) ((t2.tv_sec -t1.tv_sec) *1000+ \
                           (t2.tv_usec-t1.tv_usec)/1000)

   if(data[2] & 0x04) {         /* FINGER UP - Release */
      upx = avgx;               /* ignore this x, y */
      upy = avgy;               /* store Finger UP possition */
      GET_TIME(uptv);           /* set time for the next finger-down */
      tv.tv_sec = 0;            /* NO DRAG */
      avgx = -1;                /* FINGER IS UP */
      state->buttons = 0;
      return 0;
   }

   /*
    * NOW WE HAVe FINGER DOWN 
    */
   x = data[3] | (data[4] << 8);
   x &= 0xfff;
   y = data[5] | (data[6] << 8);
   x &= 0xfff;
   x = REALPOS_MAX * (x - gunze_calib[0]) / (gunze_calib[2] - gunze_calib[0]);
   y = REALPOS_MAX * (y - gunze_calib[1]) / (gunze_calib[3] - gunze_calib[1]);
   if(x < 0)
      x = 0;
   if(x > REALPOS_MAX)
      x = REALPOS_MAX;
   if(y < 0)
      y = 0;
   if(y > REALPOS_MAX)
      y = REALPOS_MAX;

   if(avgx < 0) {               /* INITIAL TOUCH, FINGER WAS UP */
      GET_TIME(tv);
      state->buttons = 0;
      if(DIF_TIME(uptv, tv) < (which_mouse->opt_time)) {        /* if Initial
                                                                 * Touch
                                                                 * immediate
                                                                 * after finger 
                                                                 * UP then
                                                                 * start DRAG */
         x = upx;
         y = upy;               /* A:start DRAG at finger-UP position */
         if(elo_click_ontouch == 0)
            state->buttons = GPM_B_LEFT;
      } else {                  /* 1:MOVE to Initial Touch position */
         upx = x;
         upy = y;               /* store position of Initial Touch into upx,
                                 * upy */
         if(elo_click_ontouch == 0)
            tv.tv_sec = 0;      /* no DRAG */
      }
      realposx = avgx = x;
      state->x = REAL_TO_XCELL(realposx);
      realposy = avgy = y;
      state->y = REAL_TO_YCELL(realposy);
      return 0;
   }

   /*
    * endof INITIAL TOUCH 
    */
   state->buttons = 0;          /* Motion event */
   if(tv.tv_sec) {              /* draging or elo_click_ontouch */
      state->buttons = GPM_B_LEFT;
      if(elo_click_ontouch) {
         x = avgx = upx;        /* 2:BUTTON PRESS at Initial Touch position */
         y = avgy = upy;
         tv.tv_sec = 0;         /* so next time 3:MOVE again until Finger UP */
      }                         /* else B:continue DRAG to current possition */
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
