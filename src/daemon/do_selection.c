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

#include "headers/message.h"        /* messaging in gpm */
#include "headers/daemon.h"         /* daemon internals */
#include "headers/gpmInt.h"         /* daemon internals */

/*-------------------------------------------------------------------*/
int do_selection(Gpm_Event *event)  /* returns 0, always */
{
   static int x1=1, y1=1, x2, y2;
#define UNPOINTER() 0

   x2=event->x; y2=event->y;
   switch(GPM_BARE_EVENTS(event->type)) {
      case GPM_MOVE:
         if (x2<1) x2++; else if (x2>maxx) x2--;
         if (y2<1) y2++; else if (y2>maxy) y2--;
         selection_copy(x2,y2,x2,y2,3); /* just highlight pointer */
         return 0;

      case GPM_DRAG:
         if (event->buttons==GPM_B_LEFT) {
            if (event->margin) /* fix margins */
               switch(event->margin) {
                  case GPM_TOP: x2=1; y2++; break;
                  case GPM_BOT: x2=maxx; y2--; break;
                  case GPM_RGT: x2--; break;
                  case GPM_LFT: y2<=y1 ? x2++ : (x2=maxx, y2--); break;
               }
            selection_copy(x1,y1,x2,y2,event->clicks);
            if (event->clicks>=opt_ptrdrag && !event->margin) /* pointer */
               selection_copy(x2,y2,x2,y2,3);
         } /* if */
         return 0;

      case GPM_DOWN:
         switch (event->buttons) {
            case GPM_B_LEFT:
               x1=x2; y1=y2;
               selection_copy(x1,y1,x2,y2,event->clicks); /* start selection */
               return 0;

            case GPM_B_MIDDLE:
               selection_paste();
               return 0;

            case GPM_B_RIGHT:
               if ((which_mouse->opt_three)==1)
                  selection_copy(x1,y1,x2,y2,event->clicks);
               else
                  selection_paste();
               return 0;
         }
   } /* switch above */
   return 0;
}

