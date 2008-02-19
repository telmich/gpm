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

#include <errno.h>                  /* guess what        */
#include <unistd.h>                 /* read              */
#include <stdlib.h>                 /* exit              */
#include <string.h>                 /* strerror          */
#include <linux/kd.h>               /* KD*               */

#include "headers/message.h"        /* messaging in gpm */
#include "headers/daemon.h"         /* daemon internals */
#include "headers/gpmInt.h"         /* daemon internals */

/*-------------------------------------------------------------------
 * fetch the actual device data from the mouse device, dependent on
 * what Gpm_Type is being passed.
 *-------------------------------------------------------------------*/
char *getMouseData(int fd, Gpm_Type *type, int kd_mode)
{
   static unsigned char data[32]; /* quite a big margin :) */
   char *edata=data+type->packetlen;
   int howmany=type->howmany;
   int i,j;

/*....................................... read and identify one byte */

   if (read(fd, data, howmany)!=howmany) {
      if (opt_test) exit(0);
      gpm_report(GPM_PR_ERR,GPM_MESS_READ_FIRST, strerror(errno));
      return NULL;
   }

   if (kd_mode!=KD_TEXT && fifofd != -1 && opt_rawrep)
      write(fifofd, data, howmany);

   if ((data[0]&((which_mouse->m_type)->proto)[0]) != ((which_mouse->m_type)->proto)[1]) {
      if ((which_mouse->m_type)->getextra == 1) {
         data[1]=GPM_EXTRA_MAGIC_1; data[2]=GPM_EXTRA_MAGIC_2;
         gpm_report(GPM_PR_DEBUG,GPM_EXTRA_DATA,data[0]);
         return data;
      }
      gpm_report(GPM_PR_DEBUG,GPM_MESS_PROT_ERR);
      return NULL;
   }

/*....................................... read the rest */

   /*
    * well, this seems to work almost right with ps2 mice. However, I've never
    * tried ps2 with the original selection package, which called usleep()
    */

   if((i=(which_mouse->m_type)->packetlen-howmany)) /* still to get */
      do {
         j = read(fd,edata-i,i); /* edata is pointer just after data */
         if (kd_mode!=KD_TEXT && fifofd != -1 && opt_rawrep && j > 0)
            write(fifofd, edata-i, j);
         i -= j;
      } while (i && j);

   if (i) {
      gpm_report(GPM_PR_ERR,GPM_MESS_READ_REST, strerror(errno));
      return NULL;
   }

   if ((data[1]&((which_mouse->m_type)->proto)[2]) != ((which_mouse->m_type)->proto)[3]) {
      gpm_report(GPM_PR_INFO,GPM_MESS_SKIP_DATA);
      return NULL;
   }
   gpm_report(GPM_PR_DEBUG,GPM_MESS_DATA_4,data[0],data[1],data[2],data[3]);
   return data;
}

