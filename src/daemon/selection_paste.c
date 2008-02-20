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

#include <time.h>                   /* time              */
#include <fcntl.h>                  /* open              */
#include <unistd.h>                 /* close             */

#include "headers/message.h"        /* messaging in gpm */
#include "headers/daemon.h"         /* daemon internals */

void selection_paste(void)
{
   char c=3;
   int fd;

   if (!opt_aged && (0 != opt_age_limit) &&
      (last_selection_time + opt_age_limit < time(0))) {
      opt_aged = 1;
   }

   if (opt_aged) {
      gpm_report(GPM_PR_DEBUG,GPM_MESS_SKIP_PASTE);
      return;
   }

   fd=open_console(O_WRONLY);
   if(ioctl(fd, TIOCLINUX, &c) < 0)
      gpm_report(GPM_PR_OOPS,GPM_MESS_IOCTL_TIOCLINUX);
   close(fd);
}

