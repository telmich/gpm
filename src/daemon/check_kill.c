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
#include <unistd.h>                 /* unlink            */
#include <stdlib.h>                 /* exit              */

#include "headers/message.h"        /* messaging in gpm */
#include "headers/daemon.h"         /* daemon internals */

/* itz Sat Sep 12 10:55:51 PDT 1998 Added this as replacement for the
   unwanted functionality in check_uniqueness. */

void check_kill(void)
{
   int old_pid;
   FILE* fp = fopen(GPM_NODE_PID, "r");

   /* if we cannot find the old pid file, leave */
   if (fp == NULL) gpm_report(GPM_PR_OOPS,GPM_MESS_OPEN, GPM_NODE_PID);

   /* else read the pid */
   if (fscanf(fp,"%d",&old_pid) != 1)
      gpm_report(GPM_PR_OOPS,GPM_MESS_READ_PROB,GPM_NODE_PID);
   fclose(fp);

   gpm_report(GPM_PR_DEBUG,GPM_MESS_KILLING,old_pid);

   /* first check if we run */
   if (kill(old_pid,0) == -1) {
      gpm_report(GPM_PR_INFO,GPM_MESS_STALE_PID, GPM_NODE_PID);
      unlink(GPM_NODE_PID);
   }
   /* then kill us (not directly, but the other instance ... ) */
   if (kill(old_pid,SIGTERM) == -1)
      gpm_report(GPM_PR_OOPS,GPM_MESS_CANT_KILL, old_pid);

   gpm_report(GPM_PR_DEBUG,GPM_MESS_KILLED,old_pid);
   exit(0);
}

