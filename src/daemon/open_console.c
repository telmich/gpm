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

#include <fcntl.h>                  /* open and co.      */
#include <sys/stat.h>               /* stat()            */
#include <stropts.h>                /* ioctl             */

/* Linux specific (to be outsourced in gpm2 */
#include <linux/serial.h>           /* for serial console check */
#include <asm/ioctls.h>            /* for serial console check */


#include "headers/message.h"        /* messaging in gpm  */
#include "headers/daemon.h"         /* daemon internals  */

int open_console(const int mode)
{
   int                  fd;
   int                  maj;
   int                  twelve = 12;
   struct serial_struct si;
   struct stat          sb;

   fd = open(option.consolename, mode);
   if (fd != -1) {
      fstat(fd, &sb);
      maj = major(sb.st_rdev);
      if (maj != 4 && (maj < 136 || maj > 143)) {
          if (ioctl(fd, TIOCLINUX, &twelve) < 0) {
              if (si.line > 0) {
                  gpm_report(GPM_PR_OOPS,GPM_MESS_OPEN_SERIALCON);
               }
          }
      }
   } else
      gpm_report(GPM_PR_OOPS,GPM_MESS_OPEN_CON);
   return fd;
}
