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

#include <fcntl.h>                  /* open              */
#include <unistd.h>                 /* close             */
#include <time.h>                   /* time              */


#include "headers/message.h"        /* messaging in gpm */
#include "headers/daemon.h"         /* daemon internals */

void selection_copy(int x1, int y1, int x2, int y2, int mode)
{
/*
 * The approach in "selection" causes a bus error when run under SunOS 4.1
 * due to alignment problems...
 */
   unsigned char buf[6*sizeof(short)];
   unsigned short *arg = (unsigned short *)buf + 1;
   int fd;

   buf[sizeof(short)-1] = 2;  /* set selection */

   arg[0]=(unsigned short)x1;
   arg[1]=(unsigned short)y1;
   arg[2]=(unsigned short)x2;
   arg[3]=(unsigned short)y2;
   arg[4]=(unsigned short)mode;

   if ((fd=open_console(O_WRONLY))<0)
      gpm_report(GPM_PR_OOPS,GPM_MESS_OPEN_CON);
   /* FIXME: should be replaced with string constant (headers/message.h) */
   gpm_report(GPM_PR_DEBUG,"ctl %i, mode %i",(int)*buf, arg[4]);
   if (ioctl(fd, TIOCLINUX, buf+sizeof(short)-1) < 0)
     gpm_report(GPM_PR_OOPS,GPM_MESS_IOCTL_TIOCLINUX);
   close(fd);

   if (mode < 3) {
      opt_aged = 0;
      last_selection_time = time(0);
   }
}

