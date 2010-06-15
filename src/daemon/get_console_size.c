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

#include <unistd.h>                 /* close             */
#include <fcntl.h>                  /* open              */

#include "headers/message.h"        /* messaging in gpm */
#include "headers/daemon.h"         /* daemon internals */
#include "headers/gpmInt.h"         /* daemon internals */

void get_console_size(Gpm_Event *ePtr)
{
   int i, prevmaxx, prevmaxy;
   struct mouse_features *which_mouse; /* local */

   /* before asking the new console size, save the previous values */
   prevmaxx = maxx; prevmaxy = maxy;

   i=open_console(O_RDONLY);
   ioctl(i, TIOCGWINSZ, &win);
   close(i);
   if (!win.ws_col || !win.ws_row) {
      gpm_report(GPM_PR_DEBUG,GPM_MESS_ZERO_SCREEN_DIM);
      win.ws_col=80; win.ws_row=25;
   }
   maxx=win.ws_col; maxy=win.ws_row;
   gpm_report(GPM_PR_DEBUG,GPM_MESS_SCREEN_SIZE,maxx,maxy);

   if (!prevmaxx) { /* first invocation, place the pointer in the middle */
      statusX = ePtr->x = maxx/2;
      statusY = ePtr->y = maxy/2;
   } else { /* keep the pointer in the same position where it was */
      statusX = ePtr->x = ePtr->x * maxx / prevmaxx;
      statusY = ePtr->y = ePtr->y * maxy / prevmaxy;
   }

   for (i=1; i <= 1+opt_double; i++) {
      which_mouse=mouse_table+i; /* used to access options */
     /*
      * the following operation is based on the observation that 80x50
      * has square cells. (An author-centric observation ;-)
      */
     (which_mouse->opt_scaley)=(which_mouse->opt_scale)*50*maxx/80/maxy;
     gpm_report(GPM_PR_DEBUG,GPM_MESS_X_Y_VAL,(which_mouse->opt_scale),(which_mouse->opt_scaley));
   }
}

