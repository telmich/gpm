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

#include <signal.h>                 /* kill              */
#include <unistd.h>                 /* kill, getpid      */

#include "headers/message.h"        /* messaging in gpm */
#include "headers/daemon.h"         /* daemon internals */

/* itz Sat Sep 12 10:30:05 PDT 1998 this function used to mix two
   completely different things; opening a socket to a running daemon
   and checking that a running daemon existed.  Ugly. */
/* rewritten mostly on 20th of February 2002 - nico */
void check_uniqueness(void)
{
   FILE *fp    =  0;
   int old_pid = -1;

   if((fp = fopen(GPM_NODE_PID, "r")) != NULL) {
      fscanf(fp, "%d", &old_pid);
      if (kill(old_pid,0) == -1) {
         gpm_report(GPM_PR_INFO,GPM_MESS_STALE_PID, GPM_NODE_PID);
         unlink(GPM_NODE_PID);
      } else /* we are really running, exit asap! */
         gpm_report(GPM_PR_OOPS,GPM_MESS_ALREADY_RUN, old_pid);
   }
   /* now try to sign ourself */
   if ((fp = fopen(GPM_NODE_PID,"w")) != NULL) {
      fprintf(fp,"%d\n",getpid());
      fclose(fp);
   } else {
      gpm_report(GPM_PR_OOPS,GPM_MESS_NOTWRITE,GPM_NODE_PID);
   }
}

