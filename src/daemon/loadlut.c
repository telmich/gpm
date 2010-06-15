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

#include <stdint.h>                 /* datatypes         */
#include <fcntl.h>                  /* open              */
#include <stdlib.h>                 /* malloc            */
#include <string.h>                 /* str*              */
#include <errno.h>                  /* errno.h           */
#include <unistd.h>                 /* getuid            */

#include "headers/message.h"        /* messaging in gpm */
#include "headers/daemon.h"         /* daemon internals */
#include "headers/gpmInt.h"         /* GPM_SYS_CONSOLE  */

int loadlut(char *charset)
{
   int i, c, fd;
   unsigned char this, next;
   static uint32_t long_array[9]={
      0x05050505, /* ugly, but preserves alignment */
      0x00000000, /* control chars     */
      0x00000000, /* digits            */
      0x00000000, /* uppercase and '_' */
      0x00000000, /* lowercase         */
      0x00000000, /* Latin-1 control   */
      0x00000000, /* Latin-1 misc      */
      0x00000000, /* Latin-1 uppercase */
      0x00000000  /* Latin-1 lowercase */
   };


#define inwordLut (long_array+1)

   for (i=0; charset[i]; ) {
      i += getsym(charset+i, &this);
      if (charset[i] == '-' && charset[i + 1] != '\0')
         i += getsym(charset+i+1, &next) + 1;
      else
         next = this;
      for (c = this; c <= next; c++)
         inwordLut[c>>5] |= 1 << (c&0x1F);
   }

   if ((fd=open(option.consolename, O_WRONLY)) < 0) {
      /* try /dev/console, if /dev/tty0 failed -- is that really senseful ??? */
      free(option.consolename); /* allocated by main */
      if((option.consolename=malloc(strlen(GPM_SYS_CONSOLE)+1)) == NULL)
         gpm_report(GPM_PR_OOPS,GPM_MESS_NO_MEM);

      /* FIXME: remove hardcoded device names */
      strcpy(option.consolename,GPM_SYS_CONSOLE);

      if ((fd=open(option.consolename, O_WRONLY)) < 0) gpm_report(GPM_PR_OOPS,GPM_MESS_OPEN_CON);
   }
   if (ioctl(fd, TIOCLINUX, &long_array) < 0) { /* fd <0 is checked */
      if (errno==EPERM && getuid())
         gpm_report(GPM_PR_WARN,GPM_MESS_ROOT); /* why do we still continue?*/
      else if (errno==EINVAL)
         gpm_report(GPM_PR_OOPS,GPM_MESS_CSELECT);
   }
   close(fd);

   return 0;
}

