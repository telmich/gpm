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
