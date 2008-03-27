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


/*
 * mice.c - mouse definitions for gpm-Linux
 *
 * Copyright (C) 1993        Andrew Haylett <ajh@gec-mrc.co.uk>
 * Copyright (C) 1994-2000   Alessandro Rubini <rubini@linux.it>
 * Copyright (C) 1998,1999   Ian Zimmerman <itz@rahul.net>
 * Copyright (C) 2001-2008   Nico Schottelius <nico-gpm2008 at schottelius.org>
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
 ********/

/*
 * This file is part of the mouse server. The information herein
 * is kept aside from the rest of the server to ease fixing mouse-type
 * issues. Each mouse type is expected to fill the `buttons', `dx' and `dy'
 * fields of the Gpm_Event structure and nothing more.
 *
 * Absolute-pointing devices (support by Marc Meis), are expecting to
 * fit `x' and `y' as well. Unfortunately, to do it the window size must
 * be accessed. The global variable "win" is available for that use.
 *
 * The `data' parameter points to a byte-array with event data, as read
 * by the mouse device. The mouse device should return a fixed number of
 * bytes to signal an event, and that exact number is read by the server
 * before calling one of these functions.
 *
 * The conversion function defined here should return 0 on success and -1
 * on failure.
 *
 * Refer to the definition of Gpm_Type to probe further.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h> /* stat() */
#include <sys/time.h> /* select() */

#include <linux/kdev_t.h> /* MAJOR */
#include <linux/keyboard.h>

/* JOYSTICK */
#ifdef HAVE_LINUX_JOYSTICK_H
#include <linux/joystick.h>
#endif

/* EV DEVICE */
#ifdef HAVE_LINUX_INPUT_H
#include <linux/input.h>
#endif /* HAVE_LINUX_INPUT_H */


#include "headers/daemon.h"

#include "headers/gpmInt.h"
#include "headers/twiddler.h"
#include "headers/synaptics.h"
#include "headers/message.h"

/*========================================================================*/
/*  real absolute coordinates for absolute devices,  not very clean */
/*========================================================================*/

#define REALPOS_MAX 16383 /*  min 0 max=16383, but due to change.  */
int realposx=-1, realposy=-1;

/*========================================================================*/
/*
 * Ok, here we are: first, provide the functions; initialization is later.
 * The return value is the number of unprocessed bytes
 */
/*========================================================================*/

#ifdef HAVE_LINUX_INPUT_H
int M_evdev (Gpm_Event * state, unsigned char *data)
{
   struct input_event thisevent;
   (void) memcpy (&thisevent, data, sizeof (struct input_event));
   if (thisevent.type == EV_REL) {
      if (thisevent.code == REL_X)
         state->dx = (signed char) thisevent.value;
      else if (thisevent.code == REL_Y)
         state->dy = (signed char) thisevent.value;
   } else if (thisevent.type == EV_KEY) {
      switch(thisevent.code) {
         case BTN_LEFT:    state->buttons ^= GPM_B_LEFT;    break;
         case BTN_MIDDLE:  state->buttons ^= GPM_B_MIDDLE;  break;
         case BTN_RIGHT:   state->buttons ^= GPM_B_RIGHT;   break;
         case BTN_SIDE:    state->buttons ^= GPM_B_MIDDLE;  break;
      }   
   }
   return 0;
}

int M_evabs (Gpm_Event * state, unsigned char *data)
{
   struct input_event thisevent;
   (void) memcpy (&thisevent, data, sizeof (struct input_event));
   if (thisevent.type == EV_ABS) {
      if (thisevent.code == ABS_X)
         state->x = thisevent.value;
      else if (thisevent.code == ABS_Y)
         state->y = thisevent.value;
   } else if (thisevent.type == EV_KEY) {
      switch(thisevent.code) {
         case BTN_LEFT:    state->buttons ^= GPM_B_LEFT;    break;
         case BTN_MIDDLE:  state->buttons ^= GPM_B_MIDDLE;  break;
         case BTN_RIGHT:   state->buttons ^= GPM_B_RIGHT;   break;
         case BTN_SIDE:    state->buttons ^= GPM_B_MIDDLE;  break;
         case BTN_TOUCH:   state->buttons ^= GPM_B_LEFT;    break;
      }
   }
   return 0;
}
#endif /* HAVE_LINUX_INPUT_H */

#define GPM_B_BOTH (GPM_B_LEFT|GPM_B_RIGHT)

/* 
 *  Wacom Tablets with pen and mouse:
 *  Relative-Mode And Absolute-Mode; 
 *  Stefan Runkel  01/2000 <runkel@runkeledv.de>,
 *  Mike Pioskowik 01/2000 <pio.d1mp@nbnet.de>
*/
/* for relative and absolute : */
int WacomModell =-1;          /* -1 means "dont know" */
int WacomAbsoluteWanted=0;    /* Tell Driver if Relative or Absolute */
int wmaxx, wmaxy;         
char upmbuf[25]; /* needed only for macro buttons of ultrapad */

/* Data for Wacom Modell Identification */
/* (MaxX, MaxY are for Modells which do not answer resolution requests */
struct WC_MODELL{
   char name[15];       
   char magic[3];
   int  maxX;
   int  maxY;
   int  border;
   int  treshold;
} wcmodell[] = {
   /* ModellName    Magic     MaxX     MaxY  Border  Tresh */
    { "UltraPad"  , "UD",        0,       0,    250,    20 }, 
 /* { "Intuos"    , "GD",        0,       0,      0,    20 }, not supported */
    { "PenPartner", "CT",        0,       0,      0,    20 }, 
    { "Graphire"  , "ET",     5103,    3711,      0,    20 }
   };

#define IsA(m) ((WacomModell==(-1))? 0:!strcmp(#m,wcmodell[WacomModell].name))

int M_wacom(Gpm_Event *state, unsigned char *data)
{
   static int ox=-1, oy;
   int x, y;
   int macro=0;      /* macro buttons from tablet */

   /* Bit [0]&64 seems to have different meanings -
    *   graphire:   inside of active area;
    *   Ultrapad:   Pen in proximity (means: pen detected)
    *   Penpartner: no idea...
    * this may be of worth sometimes, but for now, we can say, that
    * if the graphire tells us that we are in the active area, we can tell
    * also that the graphire has the pen in proximity.
    */

   if (!(data[0]&64)) { /* Tool not in proximity or out of active area */
   /* handle Ultrapad macro buttons, if we get a packet without
    * proximity bit but with the buttonflag set, we know that we have
    * a macro event */
      if (IsA(UltraPad)) {
         if (data[0]&8) { /* here: macro button has been pressed */
            if (data[3]&8)  macro=(data[6]); 
            if (data[3]&16) macro=(data[6])+12;
            if (data[3]&32) macro=(data[6])+24;  /* rom-version >= 1.3 */

            state->modifiers=macro;
            /* Here we simulate the middle mousebutton */
            /* with ultrapad Eprom Version 1.2 */
            /* WHY IS THE FOLLOWING CODE DISABLE ? FIXME
            gpm_report(GPM_PR_INFO,GPM_MESS_WACOM_MACRO, macro); 
            if (macro==12) state->buttons =  GPM_B_MIDDLE;
            */
         } /* end if macrobutton pressed */
      } /* end if ultrapad */
          
      if (!IsA(UltraPad)){ /* Tool out of active area */ 
         ox=-1; 
         state->buttons=0; 
         state->dx=state->dy=0; 
      }

      return 0; /* nothing more to do so leave */
   } /* end if Tool out of active area*/ 

   x = (((data[0] & 0x3) << 14) +  (data[1] << 7) + data[2]); 
   y = (((data[3] & 0x3) << 14) + (data[4] << 7) + data[5]);
   
   if (WacomAbsoluteWanted) { /* Absolute Mode */
      if (x>wmaxx) x=wmaxx; if (x<0) x=0;
      if (y>wmaxy) y=wmaxy; if (y<0) y=0;
      state->x  = (x * win.ws_col / wmaxx);
      state->y  = (y * win.ws_row / wmaxy);
       
      realposx = (x / wmaxx); /* this two lines come from the summa driver. */
      realposy = (y / wmaxy); /* they seem to be buggy (always give zero).  */

   } else {                                               /* Relative Mode */
      /* Treshold; if greather then treat tool as first time in proximity */
      if( abs(x-ox)>(wmaxx/wcmodell[WacomModell].treshold) 
       || abs(y-oy)>(wmaxy/wcmodell[WacomModell].treshold) ) ox=x; oy=y;

      state->dx= (x-ox) / (wmaxx / win.ws_col / wcmodell[WacomModell].treshold);
      state->dy= (y-oy) / (wmaxy / win.ws_row / wcmodell[WacomModell].treshold);
   }

   ox=x; oy=y;    
   
   state->buttons=                      /* for Ultra-Pad and graphire */
      !!(data[3]&8)  * GPM_B_LEFT  +
      !!(data[3]&16) * GPM_B_RIGHT +
      !!(data[3]&32) * GPM_B_MIDDLE;   /* UD: rom-version >=1.3 */
   return 0;
}

int M_twid(Gpm_Event *state,  unsigned char *data)
{
   unsigned long message=0UL; int i,h,v;
   static int lasth, lastv, lastkey, key, lock=0, autorepeat=0;

   /* build the message as a single number */
   for (i=0; i<5; i++)
      message |= (data[i]&0x7f)<<(i*7);
   key = message & TW_ANY_KEY;

   if ((message & TW_MOD_M) == 0) { /* manage keyboard */
      if (((message & TW_ANY_KEY) != lastkey) || autorepeat)
         autorepeat = twiddler_key(message);
      lastkey = key;
      lock = 0; return -1; /* no useful mouse data */
   } 

   switch (message & TW_ANY1) {
      case TW_L1: state->buttons = GPM_B_RIGHT;  break;
      case TW_M1: state->buttons = GPM_B_MIDDLE; break;
      case TW_R1: state->buttons = GPM_B_LEFT;   break;
      case     0: state->buttons = 0;            break;
    }
   /* also, allow R1 R2 R3 (or L1 L2 L3) to be used as mouse buttons */
   if (message & TW_ANY2)  state->buttons |= GPM_B_MIDDLE;
   if (message & TW_L3)    state->buttons |= GPM_B_LEFT;
   if (message & TW_R3)    state->buttons |= GPM_B_RIGHT;

   /* put in modifiers information */
   {
      struct {unsigned long in, out;} *ptr, list[] = {
         { TW_MOD_S,  1<<KG_SHIFT },
         { TW_MOD_C,  1<<KG_CTRL  },
         { TW_MOD_A,  1<<KG_ALT   },
         { 0,         0}
      };
      for (ptr = list; ptr->in; ptr++)
         if(message & ptr->in) state->modifiers |= ptr->out;
   }

   /* now extraxt H/V */
   h = (message >> TW_H_SHIFT) & TW_M_MASK;
   v = (message >> TW_V_SHIFT) & TW_M_MASK;
   if (h & TW_M_BIT) h = -(TW_M_MASK + 1 - h);
   if (v & TW_M_BIT) v = -(TW_M_MASK + 1 - v);
  
#ifdef TWIDDLER_STATIC
   /* static implementation: return movement */
   if (!lock) {
      lasth = h;
      lastv = v;
      lock = 1;
   } 
   state->dx = -(h-lasth); lasth = h;
   state->dy = -(v-lastv); lastv = v; 

#elif defined(TWIDDLER_BALLISTIC)
   {
      /* in case I'll change the resolution */
      static int tw_threshold = 5;  /* above this it moves */
      static int tw_scale = 5;      /* every 5 report one */

      if (h > -tw_threshold && h < tw_threshold) state->dx=0;
      else {
         h = h - (h<0)*tw_threshold +lasth;
         lasth = h%tw_scale;
         state->dx = -(h/tw_scale);
      }
      if (v > -tw_threshold && v < tw_threshold) state->dy=0;
      else {
         v = v - (v<0)*tw_threshold +lastv;
         lastv = v%tw_scale;
         state->dy = -(v/tw_scale);
      }
   }

#else /* none defined: use mixed approach */
   {
      /* in case I'll change the resolution */
      static int tw_threshold = 60;    /* above this, movement is ballistic */
      static int tw_scale = 10;         /* ball: every 6 units move one unit */
      static int tw_static_scale = 3;  /* stat: every 3 units move one unit */
      static int lasthrest, lastvrest; /* integral of small motions uses rest */

      if (!lock) {
         lasth = h; lasthrest = 0;
         lastv = v; lastvrest = 0;
         lock = 1;
      } 

      if (h > -tw_threshold && h < tw_threshold) {
         state->dx = -(h-lasth+lasthrest)/tw_static_scale;
         lasthrest =  (h-lasth+lasthrest)%tw_static_scale;
      } else /* ballistic */ {
         h = h - (h<0)*tw_threshold + lasthrest;
         lasthrest = h%tw_scale;
         state->dx = -(h/tw_scale);
      }
      lasth = h;
      if (v > -tw_threshold && v < tw_threshold) {
         state->dy = -(v-lastv+lastvrest)/tw_static_scale;
         lastvrest =  (v-lastv+lastvrest)%tw_static_scale;
      } else /* ballistic */ {
         v = v - (v<0)*tw_threshold + lastvrest;
         lastvrest = v%tw_scale;
         state->dy = -(v/tw_scale);
      }
      lastv = v;
   }
#endif

   /* fprintf(stderr,"%4i %4i -> %3i %3i\n",h,v,state->dx,state->dy); */
   return 0;
}

#ifdef HAVE_LINUX_JOYSTICK_H
/* Joystick mouse emulation (David Given) */

int M_js(Gpm_Event *state,  unsigned char *data)
{
   struct JS_DATA_TYPE *jdata = (void*)data;
   static int centerx = 0;
   static int centery = 0;
   static int oldbuttons = 0;
   static int count = 0;
   int dx;
   int dy;

   count++;
   if (count < 200) {
      state->buttons = oldbuttons;
      state->dx = 0;
      state->dy = 0;
      return 0;
   }
   count = 0;

   if (centerx == 0) {
      centerx = jdata->x;
      centery = jdata->y;
   }

   state->buttons = ((jdata->buttons & 1) * GPM_B_LEFT) |
                    ((jdata->buttons & 2) * GPM_B_RIGHT);
   oldbuttons     = state->buttons;

   dx             = (jdata->x - centerx) >> 6;
   dy             = (jdata->y - centery) >> 6;

   if (dx > 0) state->dx =   dx * dx;
   else        state->dx = -(dx * dx);
   state->dx >>= 2;

   if (dy > 0) state->dy = dy * dy;
   else        state->dy = -(dy * dy);
   state->dy >>= 2;

   /* Prevent pointer drift. (PC joysticks are notoriously inaccurate.) */

   if ((state->dx >= -1) && (state->dx <= 1)) state->dx = 0;
   if ((state->dy >= -1) && (state->dy <= 1)) state->dy = 0;

   return 0;
}
#endif /* have joystick.h */

/* Synaptics TouchPad mouse emulation (Henry Davies) */
int M_synaptics_serial(Gpm_Event *state,  unsigned char *data)
{
   syn_process_serial_data (state, data);
   return 0;
}


/* Synaptics TouchPad mouse emulation (Henry Davies) */
int M_synaptics_ps2(Gpm_Event *state,  unsigned char *data)
{
   syn_process_ps2_data(state, data);
   return 0;
}

int M_mtouch(Gpm_Event *state,  unsigned char *data)
{
   /*
    * This is a simple decoder for the MicroTouch touch screen
    * devices. It uses the "tablet" format and only generates button-1
    * events. Check README.microtouch for additional information.
    */
   int x, y;
   static int avgx=-1, avgy;       /* average over time, for smooth feeling */
   static int upx, upy;            /* keep track of last finger-up place */
   static struct timeval uptv, tv; /* time of last up, and down events */

   #define REAL_TO_XCELL(x) (x * win.ws_col / 0x3FFF)
   #define REAL_TO_YCELL(y) (y * win.ws_row / 0x3FFF)

   #define GET_TIME(tv) (gettimeofday(&tv, (struct timezone *)NULL))
   #define DIF_TIME(t1,t2) ((t2.tv_sec -t1.tv_sec) *1000+ \
                           (t2.tv_usec-t1.tv_usec)/1000)

   if (!(data[0]&0x40)) {
    /*
     * finger-up event: this is usually offset a few pixels,
     * so ignore this x and y values. And invalidate avg.
     */
      upx = avgx;
      upy = avgy;
      GET_TIME(uptv); /* ready for the next finger-down */
      tv.tv_sec = 0;
      state->buttons = 0;
      avgx=-1; /* invalidate avg */
      return 0;
   }

   x =           data[1] | (data[2]<<7);
   y = 0x3FFF - (data[3] | (data[4]<<7));

   if (avgx < 0) { /* press event */
      GET_TIME(tv);
      if (DIF_TIME(uptv, tv) < (which_mouse->opt_time)) {
         /* count as button press placed at finger-up pixel */
         state->buttons = GPM_B_LEFT;
         realposx = avgx = upx; state->x = REAL_TO_XCELL(realposx);
         realposy = avgy = upy; state->y = REAL_TO_YCELL(realposy);
         upx = (upx - x); /* upx and upy become offsets to use for this drag */
         upy = (upy - y);
         return 0;
      }
      /* else, count as a new motion event */
      tv.tv_sec = 0; /* invalidate */
      realposx = avgx = x; state->x = REAL_TO_XCELL(realposx);
      realposy = avgy = y; state->y = REAL_TO_YCELL(realposy);
   }

   state->buttons = 0;
   if (tv.tv_sec) { /* a drag event: use position relative to press */
      x += upx;
      y += upy;
      state->buttons = GPM_B_LEFT;
   }

   realposx = avgx = (9*avgx + x)/10; state->x = REAL_TO_XCELL(realposx);
   realposy = avgy = (9*avgy + y)/10; state->y = REAL_TO_YCELL(realposy);

   return 0;

   #undef REAL_TO_XCELL
   #undef REAL_TO_YCELL
   #undef GET_TIME
   #undef DIF_TIME
}

/*
 * This decoder is copied and adapted from the above mtouch.
 * However, this one uses argv, which should be ported above as well.
 * These variables are used for option passing
 */
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

/*
 * This decoder is copied and adapted from the above mtouch.
 */
int elo_click_ontouch = 0; /* the bigger the smoother */
int M_etouch(Gpm_Event *state,  unsigned char *data)
{ /*
   * This is a simple decoder for the EloTouch touch screen devices. 
   * ELO format SmartSet UTsXXYYZZc 9600,N,8,1
   * c=checksum = 0xAA+'T'+'U'+s+X+X+Y+Y+Z+Z (XXmax=YYmax=0x0FFF=4095)
   * s=status   bit 0=init touch  1=stream touch  2=release
   */
#define ELO_CLICK_ONTOUCH	/* ifdef then ButtonPress on first Touch 
				         else first Move then Touch*/
  int x, y;
  static int avgx=-1, avgy;       /* average over time, for smooth feeling */
  static int upx, upy;            /* keep track of last finger-up place */
  static struct timeval uptv, tv; /* time of last up, and down events */

  #define REAL_TO_XCELL(x) (x * win.ws_col / 0x3FFF)
  #define REAL_TO_YCELL(y) (y * win.ws_row / 0x3FFF)

  #define GET_TIME(tv) (gettimeofday(&tv, (struct timezone *)NULL))
  #define DIF_TIME(t1,t2) ((t2.tv_sec -t1.tv_sec) *1000+ \
                           (t2.tv_usec-t1.tv_usec)/1000)

  if (data[2]&0x04)	/* FINGER UP - Release */
  { upx = avgx;		/* ignore this x, y */
    upy = avgy;		/* store Finger UP possition */
    GET_TIME(uptv);	/* set time for the next finger-down */
    tv.tv_sec = 0;	/* NO DRAG */
    avgx=-1;		/* FINGER IS UP */
    state->buttons = 0;
    return 0;
  }

  /* NOW WE HAVe FINGER DOWN */
  x = data[3] | (data[4]<<8); x&=0xfff;
  y = data[5] | (data[6]<<8); x&=0xfff;
  x = REALPOS_MAX * (x - gunze_calib[0])/(gunze_calib[2]-gunze_calib[0]);
  y = REALPOS_MAX * (y - gunze_calib[1])/(gunze_calib[3]-gunze_calib[1]);
  if (x<0) x = 0; if (x > REALPOS_MAX) x = REALPOS_MAX;
  if (y<0) y = 0; if (y > REALPOS_MAX) y = REALPOS_MAX;

  if (avgx < 0)		/* INITIAL TOUCH, FINGER WAS UP */
  { GET_TIME(tv);
    state->buttons = 0;
    if (DIF_TIME(uptv, tv) < (which_mouse->opt_time))
    {	/* if Initial Touch immediate after finger UP then start DRAG */
	x=upx; y=upy;  /* A:start DRAG at finger-UP position */ 
	if (elo_click_ontouch==0)	state->buttons = GPM_B_LEFT;
    }
    else /*  1:MOVE to Initial Touch position */
    {	upx=x; upy=y; /* store position of Initial Touch into upx, upy */
	if (elo_click_ontouch==0)	tv.tv_sec=0; /* no DRAG */
    }
    realposx = avgx = x; state->x = REAL_TO_XCELL(realposx);
    realposy = avgy = y; state->y = REAL_TO_YCELL(realposy);
    return 0;
  } /* endof INITIAL TOUCH */


  state->buttons = 0; /* Motion event */
  if (tv.tv_sec)	/* draging or elo_click_ontouch */
  { state->buttons = GPM_B_LEFT;
    if (elo_click_ontouch) 
    {	x=avgx=upx;	/* 2:BUTTON PRESS at Initial Touch position */
	y=avgy=upy;
	tv.tv_sec=0;   /* so next time 3:MOVE again until Finger UP*/
    } /* else B:continue DRAG to current possition */
  }

  realposx = avgx = (9*avgx + x)/10; state->x = REAL_TO_XCELL(realposx);
  realposy = avgy = (9*avgy + y)/10; state->y = REAL_TO_YCELL(realposy);
  return 0;

  #undef REAL_TO_XCELL
  #undef REAL_TO_YCELL
  #undef GET_TIME
  #undef DIF_TIME
}


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

/*========================================================================*/
/* Then, mice should be initialized */

Gpm_Type* I_empty(int fd, unsigned short flags,
    struct Gpm_Type *type, int argc, char **argv)
{
    if (check_no_argv(argc, argv)) return NULL;
    return type;
}

int setspeed(int fd,int old,int new,int needtowrite,unsigned short flags)
{
   struct termios tty;
   char *c;

   tcgetattr(fd, &tty);
  
   tty.c_iflag = IGNBRK | IGNPAR;
   tty.c_oflag = 0;
   tty.c_lflag = 0;
   tty.c_line = 0;
   tty.c_cc[VTIME] = 0;
   tty.c_cc[VMIN] = 1;

   switch (old) {
      case 19200: tty.c_cflag = flags | B19200; break;
      case  9600: tty.c_cflag = flags |  B9600; break;
      case  4800: tty.c_cflag = flags |  B4800; break;
      case  2400: tty.c_cflag = flags |  B2400; break;
      case  1200:
      default:    tty.c_cflag = flags | B1200; break;
   }

   tcsetattr(fd, TCSAFLUSH, &tty);

   switch (new) {
      case 19200: c = "*r";  tty.c_cflag = flags | B19200; break;
      case  9600: c = "*q";  tty.c_cflag = flags |  B9600; break;
      case  4800: c = "*p";  tty.c_cflag = flags |  B4800; break;
      case  2400: c = "*o";  tty.c_cflag = flags |  B2400; break;
      case  1200:
      default:    c = "*n";  tty.c_cflag = flags | B1200; break;
   }

   if (needtowrite) write(fd, c, 2);
   usleep(100000);
   tcsetattr(fd, TCSAFLUSH, &tty);
   return 0;
}



struct {
   int sample; char code[2];
} sampletab[]={
    {  0,"O"},
    { 15,"J"},
    { 27,"K"},
    { 42,"L"},
    { 60,"R"},
    { 85,"M"},
    {125,"Q"},
    {1E9,"N"}, };

Gpm_Type* I_serial(int fd, unsigned short flags,
    struct Gpm_Type *type, int argc, char **argv)
{
   int i; unsigned char c;
   fd_set set; struct timeval timeout={0,0}; /* used when not debugging */

   /* accept "-o dtr", "-o rts" and "-o both" */
   if (option_modem_lines(fd, argc, argv)) return NULL;

#ifndef DEBUG
   /* flush any pending input (thanks, Miguel) */
   FD_ZERO(&set);
   for(i=0; /* always */ ; i++) {
      FD_SET(fd,&set);
      switch(select(fd+1,&set,(fd_set *)NULL,(fd_set *)NULL,&timeout/*zero*/)){
         case  1: if (read(fd,&c,1)==0) break;
         case -1: continue;
      }
      break;
   }

   if (type->fun==M_logimsc) write(fd, "QU", 2 );

#if 0 /* Did this ever work? -- I don't know, but should we not remove it,
       * if it doesn't work ??? -- Nico */
   if (type->fun==M_ms && i==2 && c==0x33)  { /* Aha.. a mouseman... */
      gpm_report(GPM_PR_INFO,GPM_MESS_MMAN_DETECTED);
      return mice; /* it is the first */
   }
#endif
#endif

   /* Non mman: change from any available speed to the chosen one */
   for (i=9600; i>=1200; i/=2)
      setspeed(fd, i, (which_mouse->opt_baud), (type->fun != M_mman) /* write */, flags);

   /*
    * reset the MouseMan/TrackMan to use the 3/4 byte protocol
    * (Stephen Lee, sl14@crux1.cit.cornell.edu)
    * Changed after 1.14; why not having "I_mman" now?
    */
   if (type->fun==M_mman) {
      setspeed(fd, 1200, 1200, 0, flags); /* no write */
      write(fd, "*X", 2);
      setspeed(fd, 1200, (which_mouse->opt_baud), 0, flags); /* no write */
      return type;
   }

   if(type->fun==M_geni) {
      gpm_report(GPM_PR_INFO,GPM_MESS_INIT_GENI);
      setspeed(fd, 1200, 9600, 1, flags); /* write */
      write(fd, ":" ,1); 
      write(fd, "E" ,1); /* setup tablet. relative mode, resolution... */
      write(fd, "@" ,1); /* setup tablet. relative mode, resolution... */
    }

   if (type->fun==M_synaptics_serial) {
      int packet_length;

      setspeed (fd, 1200, 1200, 1, flags);
      packet_length = syn_serial_init (fd);
      setspeed (fd, 1200, 9600, 1, flags);

      type->packetlen = packet_length;
      type->howmany   = packet_length;
    }

   if (type->fun==M_vsxxx_aa) {
      setspeed (fd, 4800, 4800, 0, flags); /* no write */
      write(fd, "R", 1); /* initialize a mouse; without getting an "R" */
                         /* a mouse does not send a bytestream         */
   }

   return type;
}

Gpm_Type* I_logi(int fd, unsigned short flags,
       struct Gpm_Type *type, int argc, char **argv)
{
   int i;
   struct stat buf;
   int busmouse;

   if (check_no_argv(argc, argv)) return NULL;

   /* is this a serial- or a bus- mouse? */
   if(fstat(fd,&buf)==-1) gpm_report(GPM_PR_OOPS,GPM_MESS_FSTAT);
   i=MAJOR(buf.st_rdev);
   
   /* I don't know why this is herein, but I remove it. I don't think a 
    * hardcoded ttyname in a C file is senseful. I think if the device 
    * exists must be clear before. Not here. */
   /********* if (stat("/dev/ttyS0",&buf)==-1) gpm_oops("stat()"); *****/
   busmouse=(i != MAJOR(buf.st_rdev));

   /* fix the howmany field, so that serial mice have 1, while busmice have 3 */
   type->howmany = busmouse ? 3 : 1;

   /* change from any available speed to the chosen one */
   for (i=9600; i>=1200; i/=2) setspeed(fd, i, (which_mouse->opt_baud), 1 /* write */, flags);

   /* this stuff is peculiar of logitech mice, also for the serial ones */
   write(fd, "S", 1);
   setspeed(fd, (which_mouse->opt_baud), (which_mouse->opt_baud), 1 /* write */,
      CS8 |PARENB |PARODD |CREAD |CLOCAL |HUPCL);

   /* configure the sample rate */
   for (i=0;(which_mouse->opt_sample)<=sampletab[i].sample;i++) ;
   write(fd,sampletab[i].code,1);
   return type;
}

Gpm_Type *I_wacom(int fd, unsigned short flags,
                         struct Gpm_Type *type, int argc, char **argv)
{
/* wacom graphire tablet */
#define UD_RESETBAUD     "\r$"      /* reset baud rate to default (wacom V) */
                                    /* or switch to wacom IIs (wacomIV)     */ 
#define UD_RESET         "#\r"      /* Reset tablet and enable WACOM IV     */
#define UD_SENDCOORDS    "ST\r"     /* Start sending coordinates            */
#define UD_FIRMID        "~#\r"     /* Request firmware ID string           */
#define UD_COORD         "~C\r"     /* Request max coordinates              */
#define UD_STOP          "\nSP\r"   /* stop sending coordinates             */

   void reset_wacom()
   { 
      /* Init Wacom communication; this is modified from xf86Wacom.so module */
      /* Set speed to 19200 */
      setspeed (fd, 1200, 19200, 0, B19200|CS8|CREAD|CLOCAL|HUPCL);
      /* Send Reset Baudrate Command */ 
      write(fd, UD_RESETBAUD, strlen(UD_RESETBAUD));
      usleep(250000);   
      /* Send Reset Command */
      write(fd, UD_RESET,     strlen(UD_RESET));
      usleep(75000);
      /* Set speed to 9600bps */
      setspeed (fd, 1200, 9600, 0, B9600|CS8|CREAD|CLOCAL|HUPCL);
      /* Send Reset Command */
      write(fd, UD_RESET, strlen(UD_RESET));
      usleep(250000);  
      write(fd, UD_STOP, strlen(UD_STOP));
      usleep(100000);
   }

   int wait_wacom()
   {
      /* 
       *  Wait up to 200 ms for Data from Tablet. 
       *  Do not read that data.
       *  Give back 0 on timeout condition, -1 on error and 1 for DataPresent
       */
      struct timeval timeout;
      fd_set readfds;
      int err;
      FD_ZERO(&readfds);  FD_SET(fd, &readfds);
      timeout.tv_sec = 0; timeout.tv_usec = 200000;
      err = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);
      return((err>0)?1:err);
   }

   char buffer[50], *p;

   int RequestData(char *cmd)
   {
      int err;
      /* 
       * Send cmd if not null, and get back answer from tablet. 
       * Get Data to buffer until full or timeout.
       * Give back 0 for timeout and !0 for buffer full
       */
      if (cmd) write(fd,cmd,strlen(cmd));
      memset(buffer,0,sizeof(buffer)); p=buffer;
      err=wait_wacom();
      while (err != -1 && err && (p-buffer)<(sizeof(buffer)-1)) {
         p+= read(fd,p,(sizeof(buffer)-1)-(p-buffer));
         err=wait_wacom();
      }
      /* return 1 for buffer full */
      return ((strlen(buffer) >= (sizeof(buffer)-1))? !0 :0);
   }

   /*
    * We do both modes, relative and absolute, with the same function.
    * If WacomAbsoluteWanted is !0 then that function calculates
    * absolute values, else relative ones.
    * We set this by the -o switch:
    * gpm -t wacom -o absolute   # does absolute mode
    * gpm -t wacom               # does relative mode
    * gpm -t wacom -o relative   # does relative mode
    */

   /* accept boolean options absolute and relative */
   static argv_helper optioninfo[] = {
         {"absolute",  ARGV_BOOL, u: {iptr: &WacomAbsoluteWanted}, value: !0},
         {"relative",  ARGV_BOOL, u: {iptr: &WacomAbsoluteWanted}, value:  0},
         {"",       ARGV_END} 
   };
   parse_argv(optioninfo, argc, argv); 
   type->absolute = WacomAbsoluteWanted;
   reset_wacom();

   /* "Flush" input queque */
   while(RequestData(NULL)) ;

   /* read WACOM-ID */
   RequestData(UD_FIRMID); 

   /* Search for matching modell */
   for(WacomModell=0;
      WacomModell< (sizeof(wcmodell) / sizeof(struct WC_MODELL));
      WacomModell++ ) {
      if (!strncmp(buffer+2,wcmodell[WacomModell].magic, 2)) {
         /* Magic matches, modell found */
         wmaxx=wcmodell[WacomModell].maxX;
         wmaxy=wcmodell[WacomModell].maxY;
         break;
      }
   }
   if(WacomModell >= (sizeof(wcmodell) / sizeof(struct WC_MODELL))) 
      WacomModell=-1;
   gpm_report(GPM_PR_INFO,GPM_MESS_WACOM_MOD, type->absolute? 'A':'R',
                (WacomModell==(-1))? "Unknown" : wcmodell[WacomModell].name,
                buffer+2);
   
   /* read Wacom max size */
   if(WacomModell!=(-1) && (!wcmodell[WacomModell].maxX)) {
      RequestData(UD_COORD);
      sscanf(buffer+2, "%d,%d", &wmaxx, &wmaxy);
      wmaxx = (wmaxx-wcmodell[WacomModell].border);
      wmaxy = (wmaxy-wcmodell[WacomModell].border);
   }
   write(fd,UD_SENDCOORDS,4);

   return type;
}

Gpm_Type *I_pnp(int fd, unsigned short flags,
             struct Gpm_Type *type, int argc, char **argv)
{  
   struct termios tty;

   /* accept "-o dtr", "-o rts" and "-o both" */
   if (option_modem_lines(fd, argc, argv)) return NULL;

   /*
    * Just put the device to 1200 baud. Thanks to Francois Chastrette
    * for his great help and debugging with his own pnp device.
    */
   tcgetattr(fd, &tty);
    
   tty.c_iflag = IGNBRK | IGNPAR;
   tty.c_oflag = 0;
   tty.c_lflag = 0;
   tty.c_line = 0;
   tty.c_cc[VTIME] = 0;
   tty.c_cc[VMIN] = 1;
   tty.c_cflag = flags | B1200;
   tcsetattr(fd, TCSAFLUSH, &tty); /* set parameters */

   /*
    * Don't read the silly initialization string. I don't want to see
    * the vendor name: it is only propaganda, with no information.
    */
   
   return type;
}

/*
 * Sends the SEND_ID command to the ps2-type mouse.
 * Return one of GPM_AUX_ID_...
 */
int read_mouse_id(int fd)
{
   unsigned char c = GPM_AUX_SEND_ID;
   unsigned char id;

   write(fd, &c, 1);
   read(fd, &c, 1);
   if (c != GPM_AUX_ACK) {
      return(GPM_AUX_ID_ERROR);
   }
   read(fd, &id, 1);

   return(id);
}

/**
 * Writes the given data to the ps2-type mouse.
 * Checks for an ACK from each byte.
 * 
 * Returns 0 if OK, or >0 if 1 or more errors occurred.
 */
int write_to_mouse(int fd, unsigned char *data, size_t len)
{
   int i;
   int error = 0;
   for (i = 0; i < len; i++) {
      unsigned char c;
      write(fd, &data[i], 1);
      read(fd, &c, 1);
      if (c != GPM_AUX_ACK) error++;
   }

   /* flush any left-over input */
   usleep (30000);
   tcflush (fd, TCIFLUSH);
   return(error);
}


/* intellimouse, ps2 version: Ben Pfaff and Colin Plumb */
/* Autodetect: Steve Bennett */
Gpm_Type *I_imps2(int fd, unsigned short flags, struct Gpm_Type *type,
                                                       int argc, char **argv)
{
   int id;
   static unsigned char basic_init[] = { GPM_AUX_ENABLE_DEV, GPM_AUX_SET_SAMPLE, 100 };
   static unsigned char imps2_init[] = { GPM_AUX_SET_SAMPLE, 200, GPM_AUX_SET_SAMPLE, 100, GPM_AUX_SET_SAMPLE, 80, };
   static unsigned char ps2_init[] = { GPM_AUX_SET_SCALE11, GPM_AUX_ENABLE_DEV, GPM_AUX_SET_SAMPLE, 100, GPM_AUX_SET_RES, 3, };

   /* Do a basic init in case the mouse is confused */
   write_to_mouse(fd, basic_init, sizeof (basic_init));

   /* Now try again and make sure we have a PS/2 mouse */
   if (write_to_mouse(fd, basic_init, sizeof (basic_init)) != 0) {
      gpm_report(GPM_PR_ERR,GPM_MESS_IMPS2_INIT);
      return(NULL);
   }

   /* Try to switch to 3 button mode */
   if (write_to_mouse(fd, imps2_init, sizeof (imps2_init)) != 0) {
      gpm_report(GPM_PR_ERR,GPM_MESS_IMPS2_FAILED);
      return(NULL);
   }

   /* Read the mouse id */
   id = read_mouse_id(fd);
   if (id == GPM_AUX_ID_ERROR) {
      gpm_report(GPM_PR_ERR,GPM_MESS_IMPS2_MID_FAIL);
      id = GPM_AUX_ID_PS2;
   }

   /* And do the real initialisation */
   if (write_to_mouse(fd, ps2_init, sizeof (ps2_init)) != 0) {
      gpm_report(GPM_PR_ERR,GPM_MESS_IMPS2_SETUP_FAIL);
   }

   if (id == GPM_AUX_ID_IMPS2) {
   /* Really an intellipoint, so initialise 3 button mode (4 byte packets) */
      gpm_report(GPM_PR_INFO,GPM_MESS_IMPS2_AUTO);
      return type;
   }
   if (id != GPM_AUX_ID_PS2) {
      gpm_report(GPM_PR_ERR,GPM_MESS_IMPS2_BAD_ID, id);
   }
   else gpm_report(GPM_PR_INFO,GPM_MESS_IMPS2_PS2);

   for (type=mice; type->fun; type++)
      if (strcmp(type->name, "ps2") == 0) return(type);

   /* ps2 was not found!!! */
   return(NULL);
}

Gpm_Type *I_twid(int fd, unsigned short flags,
         struct Gpm_Type *type, int argc, char **argv)
{

   if (check_no_argv(argc, argv)) return NULL;

   if (twiddler_key_init() != 0) return NULL;
   /*
   * the twiddler is a serial mouse: just drop dtr
   * and run at 2400 (unless specified differently) 
   */
   if((which_mouse->opt_baud)==DEF_BAUD) (which_mouse->opt_baud) = 2400;
   argv[1] = "dtr"; /* argv[1] is guaranteed to be NULL (this is dirty) */
   return I_serial(fd, flags, type, argc, argv);
}

Gpm_Type *I_calus(int fd, unsigned short flags,
          struct Gpm_Type *type, int argc, char **argv)
{
   if (check_no_argv(argc, argv)) return NULL;

   if ((which_mouse->opt_baud) == 1200) (which_mouse->opt_baud)=9600; /* default to 9600 */
   return I_serial(fd, flags, type, argc, argv);
}

/* synaptics touchpad, ps2 version: Henry Davies */
Gpm_Type *I_synps2(int fd, unsigned short flags,
           struct Gpm_Type *type, int argc, char **argv)
{
   syn_ps2_init (fd);
   return type;
}


Gpm_Type *I_summa(int fd, unsigned short flags,
          struct Gpm_Type *type, int argc, char **argv) 
{
   void resetsumma()
   {
      write(fd,0,1); /* Reset */
      usleep(400000); /* wait */
   }
   int waitsumma()
   {
      struct timeval timeout;
      fd_set readfds;
      int err;
      FD_ZERO(&readfds);  FD_SET(fd, &readfds);
      timeout.tv_sec = 0; timeout.tv_usec = 200000;
      err = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);
      return(err);
   }
   int err;
   char buffer[255];
   char config[5];
   /* Summasketchtablet (from xf86summa.o: thanks to Steven Lang <tiger@tyger.org>)*/
   #define SS_PROMPT_MODE  "B"     /* Prompt mode */
   #define SS_FIRMID       "z?"    /* Request firmware ID string */
   #define SS_500LPI       "h"     /* 500 lines per inch */
   #define SS_READCONFIG   "a"
   #define SS_ABSOLUTE     "F"     /* Absolute Mode */
   #define SS_TABID0       "0"     /* Tablet ID 0 */
   #define SS_UPPER_ORIGIN "b"     /* Origin upper left */
   #define SS_BINARY_FMT   "zb"    /* Binary reporting */
   #define SS_STREAM_MODE  "@"     /* Stream mode */
   /* Geniustablet (hisketch.txt)*/
   #define GEN_MMSERIES ":"
   char GEN_MODELL=0x7f;

   /* Set speed to 9600bps */
   setspeed (fd, 1200, 9600, 1, B9600|CS8|CREAD|CLOCAL|HUPCL|PARENB|PARODD);  
   resetsumma(); 
 
   write(fd, SS_PROMPT_MODE, strlen(SS_PROMPT_MODE));
  
   if (strstr(type->name,"acecad")!=NULL) summaid=11;

   if (summaid<0) { /* Summagraphics test */
      /* read the Summa Firm-ID */
      write(fd, SS_FIRMID, strlen(SS_FIRMID));
      err=waitsumma();
      if (!((err == -1) || (!err))) {
         summaid=10; /* Original Summagraphics */
         read(fd, buffer, 255); /* Read Firm-ID */
      }
   }
  
   if (summaid<0) { /* Genius-test */
      resetsumma();
      write(fd,GEN_MMSERIES,1); 
      write(fd,&GEN_MODELL,1); /* Read modell */
      err=waitsumma();
      if (!((err == -1) || (!err))) { /* read Genius-ID */
           err=waitsumma();
         if (!((err == -1) || (!err))) {
            err=waitsumma();
            if (!((err == -1) || (!err))) {
               read(fd,&config,1);
               summaid=(config[0] & 224) >> 5; /* genius tablet-id (0-7)*/
            }
         } 
      }
   } /* end of Geniustablet-test */

   /* unknown tablet ?*/
   if ((summaid<0) || (summaid==11)) { 
      resetsumma(); 
      write(fd, SS_BINARY_FMT SS_PROMPT_MODE, 3);
   }

   /* read tablet size */
   err=waitsumma();  
   if (!((err == -1) || (!err))) read(fd,buffer,sizeof(buffer));
   write(fd,SS_READCONFIG,1);
   read(fd,&config,5);
   summamaxx=(config[2]<<7 | config[1])-(SUMMA_BORDER*2);
   summamaxy=(config[4]<<7 | config[3])-(SUMMA_BORDER*2);
  
   write(fd,SS_ABSOLUTE SS_STREAM_MODE SS_UPPER_ORIGIN,3);
   if (summaid<0) write(fd,SS_500LPI SS_TABID0 SS_BINARY_FMT,4);

   return type;
}

Gpm_Type *I_mtouch(int fd, unsigned short flags,
           struct Gpm_Type *type, int argc, char **argv)
{
   struct termios tty;

   /* Set speed to 9600bps (copied from I_summa, above :) */
   tcgetattr(fd, &tty);
   tty.c_iflag = IGNBRK | IGNPAR;
   tty.c_oflag = 0;
   tty.c_lflag = 0;
   tty.c_line = 0;
   tty.c_cc[VTIME] = 0;
   tty.c_cc[VMIN] = 1;
   tty.c_cflag = B9600|CS8|CREAD|CLOCAL|HUPCL;
   tcsetattr(fd, TCSAFLUSH, &tty);


   /* Turn it to "format tablet" and "mode stream" */
   write(fd,"\001MS\r\n\001FT\r\n",10);
  
   return type;
}

/* simple initialization for the gunze touchscreen */
Gpm_Type *I_gunze(int fd, unsigned short flags,
           struct Gpm_Type *type, int argc, char **argv)
{
   struct termios tty;
   FILE *f;
   char s[80];
   int i, calibok = 0;

   #define GUNZE_CALIBRATION_FILE SYSCONFDIR "/gpm-calibration"
   /* accept a few options */
   static argv_helper optioninfo[] = {
      {"smooth",   ARGV_INT, u: {iptr: &gunze_avg}},
      {"debounce", ARGV_INT, u: {iptr: &gunze_debounce}},
      /* FIXME: add corner tapping */
      {"",       ARGV_END}
   };
   parse_argv(optioninfo, argc, argv);
    
   /* check that the baud rate is valid */
   if ((which_mouse->opt_baud) == DEF_BAUD) (which_mouse->opt_baud) = 19200; /* force 19200 as default */
   if ((which_mouse->opt_baud) != 9600 && (which_mouse->opt_baud) != 19200) {
      gpm_report(GPM_PR_ERR,GPM_MESS_GUNZE_WRONG_BAUD,option.progname, argv[0]);
      (which_mouse->opt_baud) = 19200;
   }
   tcgetattr(fd, &tty);
   tty.c_iflag = IGNBRK | IGNPAR;
   tty.c_oflag = 0;
   tty.c_lflag = 0;
   tty.c_line = 0;
   tty.c_cc[VTIME] = 0;
   tty.c_cc[VMIN] = 1;
   tty.c_cflag = ((which_mouse->opt_baud) == 9600 ? B9600 : B19200) |CS8|CREAD|CLOCAL|HUPCL;
   tcsetattr(fd, TCSAFLUSH, &tty);

   /* FIXME: try to find some information about the device */

   /* retrieve calibration, if not existent, use defaults (uncalib) */
   f = fopen(GUNZE_CALIBRATION_FILE, "r");
   if (f) {
      fgets(s, 80, f); /* discard the comment */
      if (fscanf(f, "%d %d %d %d", gunze_calib, gunze_calib+1,
         gunze_calib+2, gunze_calib+3) == 4)
         calibok = 1;
   /* Hmm... check */
      for (i=0; i<4; i++)
         if (gunze_calib[i] & ~1023) calibok = 0;
      if (gunze_calib[0] == gunze_calib[2]) calibok = 0;
      if (gunze_calib[1] == gunze_calib[3]) calibok = 0;
      fclose(f);
   }
   if (!calibok) {
      gpm_report(GPM_PR_ERR,GPM_MESS_GUNZE_CALIBRATE, option.progname);
      gunze_calib[0] = gunze_calib[1] = 128; /* 1/8 */
      gunze_calib[2] = gunze_calib[3] = 896; /* 7/8 */
   }
   return type;
}


/* simple initialization for the elo touchscreen */
Gpm_Type *I_etouch(int fd, unsigned short flags,
			  struct Gpm_Type *type, int argc, char **argv)
{
  struct termios tty;
  FILE *f;
  char s[80];
  int i, calibok = 0;

  /* Calibration config file (copied from I_gunze, below :) */
  #define ELO_CALIBRATION_FILE SYSCONFDIR "/gpm-calibration"
   /* accept a few options */
   static argv_helper optioninfo[] = {
         {"clickontouch",  ARGV_BOOL, u: {iptr: &elo_click_ontouch}, value: !0},
         {"",       ARGV_END} 
   };
   parse_argv(optioninfo, argc, argv);

  /* Set speed to 9600bps (copied from I_summa, above :) */
  tcgetattr(fd, &tty);
  tty.c_iflag = IGNBRK | IGNPAR;
  tty.c_oflag = 0;
  tty.c_lflag = 0;
  tty.c_line = 0;
  tty.c_cc[VTIME] = 0;
  tty.c_cc[VMIN] = 1;
  tty.c_cflag = B9600|CS8|CREAD|CLOCAL|HUPCL;
  tcsetattr(fd, TCSAFLUSH, &tty);

  /* retrieve calibration, if not existent, use defaults (uncalib) */
  f = fopen(ELO_CALIBRATION_FILE, "r");
  if (f) {
	fgets(s, 80, f); /* discard the comment */
	if (fscanf(f, "%d %d %d %d", gunze_calib, gunze_calib+1,
		   gunze_calib+2, gunze_calib+3) == 4)
	    calibok = 1;
	/* Hmm... check */
	for (i=0; i<4; i++)
	    if ((gunze_calib[i] & 0xfff) != gunze_calib[i]) calibok = 0;
	if (gunze_calib[0] == gunze_calib[2]) calibok = 0;
	if (gunze_calib[1] == gunze_calib[4]) calibok = 0;
	fclose(f);
  }
  if (!calibok) {
        gpm_report(GPM_PR_ERR,GPM_MESS_ELO_CALIBRATE, option.progname, ELO_CALIBRATION_FILE);
        /* "%s: etouch: calibration file %s absent or invalid, using defaults" */
	gunze_calib[0] = gunze_calib[1] = 0x010; /* 1/16 */
	gunze_calib[2] = gunze_calib[3] = 0xff0; /* 15/16 */
  }
  return type;
}



/*  Genius Wizardpad tablet  --  Matt Kimball (mkimball@xmission.com)  */
Gpm_Type *I_wp(int fd, unsigned short flags,
            struct Gpm_Type *type, int argc, char **argv)
{
   struct termios tty;
   char tablet_info[256];
   int count, pos, size;

   /* Set speed to 9600bps (copied from I_summa, above :) */
   tcgetattr(fd, &tty);
   tty.c_iflag = IGNBRK | IGNPAR;
   tty.c_oflag = 0;
   tty.c_lflag = 0;
   tty.c_line = 0;
   tty.c_cc[VTIME] = 0;
   tty.c_cc[VMIN] = 1;
   tty.c_cflag = B9600|CS8|CREAD|CLOCAL|HUPCL;
   tcsetattr(fd, TCSAFLUSH, &tty);

   /*  Reset the tablet (':') and put it in remote mode ('S') so that
     it isn't sending anything to us.  */
   write(fd, ":S", 2);
   tcsetattr(fd, TCSAFLUSH, &tty);

   /*  Query the model of the tablet  */
   write(fd, "T", 1);
   sleep(1);
   count = read(fd, tablet_info, 255);

   /*  The tablet information should start with "KW" followed by the rest of
     the model number.  If it isn't there, it probably isn't a WizardPad.  */
   if(count < 2) return NULL;
   if(tablet_info[0] != 'K' || tablet_info[1] != 'W') return NULL;

   /*  Now, we want the width and height of the tablet.  They should be 
     of the form "X###" and "Y###" where ### is the number of units of
     the tablet.  The model I've got is "X130" and "Y095", but I guess
     there might be other ones sometime.  */
   for(pos = 0; pos < count; pos++) {
      if(tablet_info[pos] == 'X' || tablet_info[pos] == 'Y') {
         if(pos + 3 < count) {
            size = (tablet_info[pos + 1] - '0') * 100 +
                   (tablet_info[pos + 2] - '0') * 10 +
                   (tablet_info[pos + 3] - '0');
            if(tablet_info[pos] == 'X') {
               wizardpad_width = size;
            } else {
               wizardpad_height = size;
            }
         }
      }
   }

   /*  Set the tablet to stream mode with 180 updates per sec.  ('O')  */
   write(fd, "O", 1);

   return type;
}

/*========================================================================*/
/* Finally, the table */
#define STD_FLG (CREAD|CLOCAL|HUPCL)

/*
 * Note that mman must be the first, and ms the second (I use this info
 * in mouse-test.c, as a quick and dirty hack
 *
 * We should clean up mouse-test and sort the table alphabeticly! --nico
 */

 /*
   * For those who are trying to add a new type, here a brief
   * description of the structure. Please refer to gpmInt.h and gpm.c
   * for more information:
   *
   * The first three strings are the name, an help line, a long name (if any)
   * Then come the functions: the decoder and the initializazion function
   *     (called I_* and M_*)
   * Follows an array of four bytes: it is the protocol-identification, based
   *     on the first two bytes of a packet: if
   *     "((byte0 & proto[0]) == proto[1]) && ((byte1 & proto[2]) == proto[3])"
   *     then we are at the beginning of a packet.
   * The following numbers are:
   *     bytes-per-packet,
   *     bytes-to-read (use 1, bus mice are pathological)
   *     has-extra-byte (boolean, for mman pathological protocol)
   *     is-absolute-coordinates (boolean)
   * Finally, a pointer to a repeater function, if any.
   */

Gpm_Type mice[]={

   {"mman", "The \"MouseMan\" and similar devices (3/4 bytes per packet).",
           "Mouseman", M_mman, I_serial, CS7 | STD_FLG, /* first */
                                {0x40, 0x40, 0x40, 0x00}, 3, 1, 1, 0, 0},
   {"ms",   "The original ms protocol, with a middle-button extension.",
           "", M_ms, I_serial, CS7 | STD_FLG,
                                {0x40, 0x40, 0x40, 0x00}, 3, 1, 0, 0, 0},
   {"acecad",  "Acecad tablet absolute mode(Sumagrapics MM-Series mode)",
           "", M_summa, I_summa, STD_FLG,
                                {0x80, 0x80, 0x00, 0x00}, 7, 1, 0, 1, 0}, 
   {"bare", "Unadorned ms protocol. Needed with some 2-buttons mice.",
           "Microsoft", M_bare, I_serial, CS7 | STD_FLG,
                                {0x40, 0x40, 0x40, 0x00}, 3, 1, 0, 0, 0},
   {"bm",   "Micro$oft busmice and compatible devices.",
           "BusMouse", M_bm, I_empty, STD_FLG, /* bm is sun */
                                {0xf8, 0x80, 0x00, 0x00}, 3, 3, 0, 0, 0},
   {"brw",  "Fellowes Browser - 4 buttons (and a wheel) (dual protocol?)",
           "", M_brw, I_pnp, CS7 | STD_FLG,
                                {0xc0, 0x40, 0xc0, 0x00}, 4, 1, 0, 0, 0},
   {"cal", "Calcomp UltraSlate",
           "", M_calus, I_calus, CS8 | CSTOPB | STD_FLG,
                                {0x80, 0x80, 0x80, 0x00}, 6, 6, 0, 1, 0},
   {"calr", "Calcomp UltraSlate - relative mode",
           "", M_calus_rel, I_calus, CS8 | CSTOPB | STD_FLG,
                                {0x80, 0x80, 0x80, 0x00}, 6, 6, 0, 0, 0},
  {"etouch",  "EloTouch touch-screens (only button-1 events, by now)",
           "", M_etouch, I_etouch, STD_FLG,
                                {0xFF, 0x55, 0xFF, 0x54}, 7, 1, 0, 1, NULL}, 
#ifdef HAVE_LINUX_INPUT_H
   {"evdev", "Linux Event Device",
            "", M_evdev, I_empty, STD_FLG,
                        {0x00, 0x00, 0x00, 0x00} , 16, 16, 0, 0, NULL},
   {"evabs", "Linux Event Device - absolute mode",
            "", M_evabs, I_empty, STD_FLG,
                        {0x00, 0x00, 0x00, 0x00} , 16, 16, 0, 1, NULL},
#endif /* HAVE_LINUX_INPUT_H */
   {"exps2",   "IntelliMouse Explorer (ps2) - 3 buttons, wheel unused",
           "ExplorerPS/2", M_imps2, I_exps2, STD_FLG,
                                {0xc0, 0x00, 0x00, 0x00}, 4, 1, 0, 0, 0},
#ifdef HAVE_LINUX_JOYSTICK_H
   {"js",   "Joystick mouse emulation",
           "Joystick", M_js, NULL, 0,
                              {0xFC, 0x00, 0x00, 0x00}, 12, 12, 0, 0, 0},
#endif
   {"genitizer", "\"Genitizer\" tablet, in relative mode.",
           "", M_geni, I_serial, CS8|PARENB|PARODD,
                                {0x80, 0x80, 0x00, 0x00}, 3, 1, 0, 0, 0},
   {"gunze",  "Gunze touch-screens (only button-1 events, by now)",
           "", M_gunze, I_gunze, STD_FLG,
                                {0xF9, 0x50, 0xF0, 0x30}, 11, 1, 0, 1, NULL}, 
   {"imps2","Microsoft Intellimouse (ps2)-autodetect 2/3 buttons,wheel unused",
           "", M_imps2, I_imps2, STD_FLG,
                                {0xC0, 0x00, 0x00, 0x00}, 4, 1, 0, 0, R_imps2},
   {"logi", "Used in some Logitech devices (only serial).",
           "Logitech", M_logi, I_logi, CS8 | CSTOPB | STD_FLG,
                                {0xe0, 0x80, 0x80, 0x00}, 3, 3, 0, 0, 0},
   {"logim",  "Turn logitech into Mouse-Systems-Compatible.",
           "", M_logimsc, I_serial, CS8 | CSTOPB | STD_FLG,
                                {0xf8, 0x80, 0x00, 0x00}, 5, 1, 0, 0, 0},
   {"mm",   "MM series. Probably an old protocol...",
           "MMSeries", M_mm, I_serial, CS8 | PARENB|PARODD | STD_FLG,
                                {0xe0, 0x80, 0x80, 0x00}, 3, 1, 0, 0, 0},
   {"ms3", "Microsoft Intellimouse (serial) - 3 buttons, wheel unused",
           "", M_ms3, I_pnp, CS7 | STD_FLG,
                                {0xc0, 0x40, 0xc0, 0x00}, 4, 1, 0, 0, R_ms3},
   {"ms+", "Like 'ms', but allows dragging with the middle button.",
           "", M_ms_plus, I_serial, CS7 | STD_FLG,
                                {0x40, 0x40, 0x40, 0x00}, 3, 1, 0, 0, 0},
   {"ms+lr", "'ms+', but you can reset m by pressing lr (see man page).",
           "", M_ms_plus_lr, I_serial, CS7 | STD_FLG,
                                {0x40, 0x40, 0x40, 0x00}, 3, 1, 0, 0, 0},
   {"msc",  "Mouse-Systems-Compatible (5bytes). Most 3-button mice.",
           "MouseSystems", M_msc, I_serial, CS8 | CSTOPB | STD_FLG,
                                {0xf8, 0x80, 0x00, 0x00}, 5, 1, 0, 0, R_msc},
   {"mtouch",  "MicroTouch touch-screens (only button-1 events, by now)",
           "", M_mtouch, I_mtouch, STD_FLG,
                                {0x80, 0x80, 0x80, 0x00}, 5, 1, 0, 1, NULL}, 
   {"ncr",  "Ncr3125pen, found on some laptops",
           "", M_ncr, NULL, STD_FLG,
                                {0x08, 0x08, 0x00, 0x00}, 7, 7, 0, 1, 0},
   {"netmouse","Genius NetMouse (ps2) - 2 buttons and 2 buttons 'up'/'down'.", 
           "", M_netmouse, I_netmouse, CS7 | STD_FLG,
                                {0xc0, 0x00, 0x00, 0x00}, 4, 1, 0, 0, 0},
   {"pnp",  "Plug and pray. New mice may not run with '-t ms'.",
           "", M_bare, I_pnp, CS7 | STD_FLG,
                                {0x40, 0x40, 0x40, 0x00}, 3, 1, 0, 0, 0},
   {"ps2",  "Busmice of the ps/2 series. Most busmice, actually.",
           "PS/2", M_ps2, I_ps2, STD_FLG,
                                {0xc0, 0x00, 0x00, 0x00}, 3, 1, 0, 0, R_ps2},
   {"sun",  "'msc' protocol, but only 3 bytes per packet.",
           "", M_sun, I_serial, CS8 | CSTOPB | STD_FLG,
                                {0xf8, 0x80, 0x00, 0x00}, 3, 1, 0, 0, R_sun},
   {"summa",  "Summagraphics or Genius tablet absolute mode(MM-Series)",
           "", M_summa, I_summa, STD_FLG,
                                {0x80, 0x80, 0x00, 0x00}, 5, 1, 0, 1, R_summa},
   {"syn", "The \"Synaptics\" serial TouchPad.",
           "synaptics", M_synaptics_serial, I_serial, CS7 | STD_FLG,
                                {0x40, 0x40, 0x40, 0x00}, 6, 6, 1, 0, 0},
   {"synps2", "The \"Synaptics\" PS/2 TouchPad",
           "synaptics_ps2", M_synaptics_ps2, I_synps2, STD_FLG,
                                {0x80, 0x80, 0x00, 0x00}, 6, 1, 1, 0, 0},
   {"twid", "Twidddler keyboard",
           "", M_twid, I_twid, CS8 | STD_FLG,
                                {0x80, 0x00, 0x80, 0x80}, 5, 1, 0, 0, 0},
   {"vsxxxaa", "The DEC VSXXX-AA/GA serial mouse on DEC workstations.",
           "", M_vsxxx_aa, I_serial, CS8 | PARENB | PARODD | STD_FLG,
                                {0xe0, 0x80, 0x80, 0x00}, 3, 1, 0, 0, 0},
   {"wacom","Wacom Protocol IV Tablets: Pen+Mouse, relative+absolute mode",
           "", M_wacom, I_wacom, STD_FLG,
                                {0x80, 0x80, 0x80, 0x00}, 7, 1, 0, 0, 0},
   {"wp",   "Genius WizardPad tablet",
           "wizardpad", M_wp, I_wp, STD_FLG,
                                {0xFA, 0x42, 0x00, 0x00}, 10, 1, 0, 1, 0},
   {"",     "",
           "", NULL, NULL, 0,
                                {0x00, 0x00, 0x00, 0x00}, 0, 0, 0, 0, 0}
};

/*------------------------------------------------------------------------*/
/* and the help */

int M_listTypes(void)
{
   Gpm_Type *type;

   printf(GPM_MESS_VERSION "\n");
   printf(GPM_MESS_AVAIL_MYT);
   for (type=mice; type->fun; type++)
      printf(GPM_MESS_SYNONYM, type->repeat_fun?'*':' ',
      type->name, type->desc, type->synonyms);

   putchar('\n');

   return 1; /* to exit() */
}

/* indent: use three spaces. no tab. not two or four. three */

/* Local Variables: */
/* c-indent-level: 3 */
/* End: */
