
/*
 * Support for the twiddler keyboard
 *
 * Copyright 1998          Alessandro Rubini (rubini at linux.it)
 * Copyright 2001 - 2008   Nico Schottelius (nico-gpm2008 at schottelius.org)
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

#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "twiddler.h"
#include "message.h"
#include "daemon.h"

/* And this uses the item to push keys */
int twiddler_use_item(const char *item)
{
   int fd = open_console(O_WRONLY);

   int i, retval = 0;

   unsigned char pushthis, unblank = 4; /* 4 == TIOCLINUX unblank */

   /*
    * a special function 
    */
   /*
    * a single byte 
    */
   if(((unsigned long) item & 0xff) == (unsigned long) item) {
      pushthis = (unsigned long) item & 0xff;
      retval = ioctl(fd, TIOCSTI, &pushthis);
   } else if(i = (struct twiddler_active_fun *) item - twiddler_active_funs,
             i >= 0 && i < twiddler_active_fun_nr)
      twiddler_do_fun(i);
   else                         /* a string */
      for(; *item != '\0' && retval == 0; item++)
         retval = ioctl(fd, TIOCSTI, item);

   ioctl(fd, TIOCLINUX, &unblank);
   if(retval)
      gpm_report(GPM_PR_ERR, GPM_MESS_IOCTL_TIOCSTI, option.progname,
                 strerror(errno));
   close(fd);
   return retval;
}
