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

#include <unistd.h>                 /* read             */

#include "headers/message.h"        /* messaging in gpm */
#include "headers/daemon.h"         /* daemon internals */
#include "headers/gpmInt.h"         /* daemon internals */

/*-------------------------------------------------------------------*
 * This  was inline, and incurred in a compiler bug (2.7.0)
 *-------------------------------------------------------------------*/
int get_data(Gpm_Connect *where, int whence)
{
   static int i;

#ifdef GPM_USE_MAGIC
   while ((i=read(whence,&check,sizeof(int)))==4 && check!=GPM_MAGIC)
      gpm_report(GPM_PR_INFO,GPM_MESS_NO_MAGIC);

   if (!i) return 0;
   if (check!=GPM_MAGIC) {
     gpm_report(GPM_PR_INFO,GPM_MESS_NOTHING_MORE);
     return -1;
   }
#endif

   if ((i=read(whence, where, sizeof(Gpm_Connect)))!=sizeof(Gpm_Connect)) {
      return i ? -1 : 0;
   }

   return 1;
}

