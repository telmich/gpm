/*
 * selection.c - GPM selection/copy/paste handling
 *
 * Copyright (C) 1993        Andreq Haylett <ajh@gec-mrc.co.uk>
 * Copyright (C) 1994-1999   Alessandro Rubini <rubini@linux.it>
 * Copyright (C) 1998        Ian Zimmerman <itz@rahul.net>
 * Copyright (c) 2001,2002   Nico Schottelius <nico-gpm@schottelius.org>
 * Copyright (c) 2003        Dmitry Torokhov <dtor@mail.ru>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>        /* strerror(); ?!?  */
#include <errno.h>
#include <unistd.h>        /* select(); */
#include <time.h>          /* time() */
#include <sys/fcntl.h>     /* O_RDONLY */
#include <sys/stat.h>      /* mkdir()  */
#include <asm/types.h>     /* __u32 */

#include <linux/vt.h>      /* VT_GETSTATE */
#include <sys/kd.h>        /* KDGETMODE */
#include <termios.h>       /* winsize */

#include "headers/gpmInt.h"
#include "headers/message.h"
#include "headers/console.h"
#include "headers/selection.h"

struct sel_options sel_opts = { 0, 0, DEF_PTRDRAG };
static time_t last_selection_time;

/*-------------------------------------------------------------------*/
static void selection_copy(int x1, int y1, int x2, int y2, int mode)
{
   /*
    * The approach in "selection" causes a bus error when run under SunOS 4.1
    * due to alignment problems...
    */
   unsigned char buf[6 * sizeof(short)];
   unsigned short *arg = (unsigned short *)buf + 1;
   int fd;

   buf[sizeof(short) - 1] = 2;  /* set selection */

   arg[0] = (unsigned short)x1;
   arg[1] = (unsigned short)y1;
   arg[2] = (unsigned short)x2;
   arg[3] = (unsigned short)y2;
   arg[4] = (unsigned short)mode;

   if ((fd = open_console(O_WRONLY)) < 0)
      gpm_report(GPM_PR_OOPS, GPM_MESS_OPEN_CON);

   gpm_report(GPM_PR_DEBUG, "ctl %i, mode %i", (int)*buf, arg[4]);
   if (ioctl(fd, TIOCLINUX, buf + sizeof(short) - 1) < 0)
      gpm_report(GPM_PR_OOPS, GPM_MESS_IOCTL_TIOCLINUX);
   close(fd);

   if (mode < 3) {
      sel_opts.aged = 0;
      last_selection_time = time(0);
   }
}

/*-------------------------------------------------------------------*/
static void selection_paste(void)
{
   char c = 3;
   int fd;

   if (!sel_opts.aged &&
      sel_opts.age_limit != 0 &&
      last_selection_time + sel_opts.age_limit < time(0)) {
      sel_opts.aged = 1;
   }

   if (sel_opts.aged) {
      gpm_report(GPM_PR_DEBUG, GPM_MESS_SKIP_PASTE);
   } else {
      fd = open_console(O_WRONLY);
      if (ioctl(fd, TIOCLINUX, &c) < 0)
         gpm_report(GPM_PR_OOPS, GPM_MESS_IOCTL_TIOCLINUX);
      close(fd);
   }
}

/*-------------------------------------------------------------------*/
void do_selection(Gpm_Event *event, int three_button_mode)
{
   static int x1 = 1, y1 = 1;
   int x2, y2;

   x2 = event->x; y2 = event->y;
   switch (GPM_BARE_EVENTS(event->type)) {
      case GPM_MOVE:
         if (x2 < 1) x2++; else if (x2 > console.max_x) x2--;
         if (y2 < 1) y2++; else if (y2 > console.max_y) y2--;
         selection_copy(x2, y2, x2, y2, 3); /* just highlight the pointer */
         break;

      case GPM_DRAG:
         if (event->buttons == GPM_B_LEFT) {
            switch (event->margin) { /* fix margins */
               case GPM_TOP: x2 = 1; y2++; break;
               case GPM_BOT: x2 = console.max_x; y2--; break;
               case GPM_RGT: x2--; break;
               case GPM_LFT: y2 <= y1 ? x2++ : (x2 = console.max_x, y2--); break;
               default: break;
            }
            selection_copy(x1, y1, x2, y2, event->clicks);
            if (event->clicks >= sel_opts.ptrdrag && !event->margin) /* pointer */
               selection_copy(x2, y2, x2, y2, 3);
         }
         break;

      case GPM_DOWN:
         switch (event->buttons) {
            case GPM_B_LEFT:
               x1 = x2; y1 = y2;
               selection_copy(x1, y1, x2, y2, event->clicks); /* start selection */
               break;

            case GPM_B_MIDDLE:
               selection_paste();
               break;

            case GPM_B_RIGHT:
               if (three_button_mode == 1)
                  selection_copy(x1, y1, x2, y2, event->clicks);
               else
                  selection_paste();
               break;
         }
   }
}

/*-------------------------------------------------------------------*/
void selection_disable_paste(void)
{
   sel_opts.aged = 1;
}
