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

#include <unistd.h>                 /* sleep()           */
#include <fcntl.h>                  /* open              */ 
#include <linux/kd.h>               /* KDGETMODE         */

#include "headers/message.h"        /* messaging in gpm  */
#include "headers/daemon.h"         /* daemon internals  */
#include "headers/gpmInt.h"         /* evil old headers  */

int wait_text(int *fdptr)
{
   int fd;
   int kd_mode;

   close(*fdptr);
   do
   {
      sleep(2);
      fd = open_console(O_RDONLY);
      if (ioctl(fd, KDGETMODE, &kd_mode)<0)
         gpm_report(GPM_PR_OOPS,GPM_MESS_IOCTL_KDGETMODE);
      close(fd);
   } while (kd_mode != KD_TEXT);

   /* reopen, reinit (the function is only used if we have one mouse device) */
   if ((*fdptr=open((which_mouse->opt_dev),O_RDWR))<0)
      gpm_report(GPM_PR_OOPS,GPM_MESS_OPEN,(which_mouse->opt_dev));
   if ((which_mouse->m_type)->init)
      (which_mouse->m_type)=((which_mouse->m_type)->init)(*fdptr, (which_mouse->m_type)->flags, (which_mouse->m_type), mouse_argc[1],
              mouse_argv[1]);
   return (1);
}
