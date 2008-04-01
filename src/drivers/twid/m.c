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

#include <linux/keyboard.h>         /* KG_SHIFT          */

#include "types.h"                  /* Gpm_type         */
#include "twiddler.h"               /* twiddler         */

int M_twid(Gpm_Event *state,  unsigned char *data)
{
   unsigned long message=0UL; int i,h,v;
   static int lasth, lastv, lastkey, key, lock=0, autorepeat=0;

   /* build the message as a single number */
   for (i=0; i<5; i++)
      message |= (data[i]&0x7f)<<(i*7);
   key = message & TW_ANY_KEY;

   if ((message & TW_MOD_M) == 0) { /* manage keyboard */
      if (((message & TW_ANY_KEY) != (unsigned long ) lastkey) || autorepeat)
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
