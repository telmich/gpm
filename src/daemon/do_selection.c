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

#include <fcntl.h>		    /* O_WRONLY */
#include <linux/kd.h>		    /* KDGKBLED */
#include <unistd.h>		    /* close()  */

#include "headers/message.h"        /* messaging in gpm */
#include "headers/daemon.h"         /* daemon internals */
#include "headers/gpmInt.h"         /* daemon internals */

/*-------------------------------------------------------------------*/
static void do_report_mouse(int fd, Gpm_Event *event, int x, int y)
{
   unsigned char buf[6*sizeof(short)];
   unsigned short *arg = (unsigned short *)buf + 1;

   const int type = event->type & (GPM_DOWN|GPM_UP);
   int buttons = event->buttons;

   if (type==GPM_UP)
      buttons = 0;

   if (type == 0 && buttons == 0)
       return;

   buf[sizeof(short)-1] = 2;  /* set selection */

   arg[0]=arg[2]=(unsigned short)x;
   arg[1]=arg[3]=(unsigned short)y;

   arg[4] =16; /* report mouse case */
   if (event->modifiers & 3)        arg[4] |= 4;  /* shift */
   if (event->modifiers & 8)        arg[4] |= 8;  /* alt */

   if (buttons & GPM_B_RIGHT)       arg[4] |= 2;
   else if (buttons & GPM_B_MIDDLE) arg[4] |= 1;
   else if (buttons & GPM_B_LEFT)   arg[4] |= 0;
   else                             arg[4] |= 3;  /* UP */

   if (ioctl(fd, TIOCLINUX, buf+sizeof(short)-1) < 0)
      gpm_report(GPM_PR_OOPS,GPM_MESS_IOCTL_TIOCLINUX);
}

/*-------------------------------------------------------------------*/
int do_selection(Gpm_Event *event)  /* returns 0, always */
{
   static int x1=1, y1=1, x2, y2;
#define UNPOINTER() 0

   int fd;
   unsigned char report_mouse = 7;

   if ((fd=open_console(O_WRONLY))<0)
      gpm_report(GPM_PR_OOPS,GPM_MESS_OPEN_CON);

   if (ioctl(fd, TIOCLINUX, &report_mouse) < 0)
      gpm_report(GPM_PR_OOPS,GPM_MESS_IOCTL_TIOCLINUX);

   if (event->modifiers & 4) /* Ctrl, keep it for selection/paste */
      report_mouse = 0;

   if (report_mouse) {
      char kbd_flag = 0;
      ioctl(fd,KDGKBLED,&kbd_flag);
      if( kbd_flag & 1 )     /* Scroll Lock */
         report_mouse = 0;
   }

   x2=event->x; y2=event->y;
   if (x2<1) x2=1; else if (x2>maxx) x2=maxx;
   if (y2<1) y2=1; else if (y2>maxy) y2=maxy;

   if (report_mouse)
      do_report_mouse(fd,event,x2,y2);

   switch(GPM_BARE_EVENTS(event->type)) {
      case GPM_MOVE:
      default:
         selection_copy(fd,x2,y2,x2,y2,3); /* just highlight pointer */
         break;

      case GPM_DRAG:
         if (event->buttons==GPM_B_LEFT && !report_mouse)
            selection_copy(fd,x1,y1,x2,y2,event->clicks);
         if ((event->clicks>=opt_ptrdrag && !event->margin) || report_mouse)
            selection_copy(fd,x2,y2,x2,y2,3); /* pointer */
         break;

      case GPM_DOWN:
         if (!report_mouse)
         switch (event->buttons) {
            case GPM_B_LEFT:
	       x1=x2; y1=y2;
	       selection_copy(fd,x1,y1,x2,y2,event->clicks);  /*  start selection */
               break;

            case GPM_B_MIDDLE:
               selection_paste(fd);
               break;

            case GPM_B_RIGHT:
               if ((which_mouse->opt_three)==1)
	          selection_copy(fd,x1,y1,x2,y2,event->clicks);
	       else
	          selection_paste(fd);
	       break;
         }
         break;

   } /* switch above */

   close(fd);
   return 0;
}
