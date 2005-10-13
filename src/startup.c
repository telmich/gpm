/*
 * general purpose mouse support for Linux
 *
 * *Startup and Daemon functions*
 *
 * Copyright (c) 2002         Nico Schottelius <nico-gpm@schottelius.org>
 *
 * 
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

#include <stdlib.h>     /* atexit() */
#include <string.h>     /* strlen() */
#include <errno.h>      /* errno */
#include <unistd.h>     /* unlink,geteuid */
#include <signal.h>
#include <sys/types.h>  /* geteuid, mknod */
#include <sys/stat.h>   /* mknod */
#include <fcntl.h>      /* mknod */
#include <unistd.h>     /* mknod */


#include "headers/gpmInt.h"
#include "headers/message.h"
#include "headers/console.h"
#include "headers/selection.h"

/* what todo atexit */
static void gpm_exited(void)
{
   gpm_report(GPM_PR_DEBUG, GPM_MESS_REMOVE_FILES, GPM_NODE_PID, GPM_NODE_CTL);
   unlink(GPM_NODE_PID);
   unlink(GPM_NODE_CTL);
}

void startup(int argc, char **argv)
{
   extern struct options option;
   extern int errno;

   /* basic settings */
   option.run_status    = GPM_RUN_STARTUP;      /* 10,9,8,... let's go */
   option.autodetect    = 0;                    /* no mouse autodection */
   option.progname      = argv[0];              /* who we are */

   get_console_name();
   cmdline(argc, argv);                         /* parse command line */

   if (geteuid() != 0) gpm_report(GPM_PR_OOPS,GPM_MESS_ROOT); /* root or exit */

   /* Planned for gpm-1.30, but only with devfs */
   /* if(option.autodetect) autodetect(); */

   
   /****************** OLD CODE from gpn.c ***********************/
   
   openlog(option.progname, LOG_PID,
           option.run_status != GPM_RUN_DEBUG ? LOG_DAEMON : LOG_USER);

   console_load_lut();

   if (repeater.raw || repeater.type) {
      if (mkfifo(GPM_NODE_FIFO, 0666) && errno != EEXIST)
         gpm_report(GPM_PR_OOPS, GPM_MESS_CREATE_FIFO, GPM_NODE_FIFO);
      if ((repeater.fd = open(GPM_NODE_FIFO, O_RDWR|O_NONBLOCK)) < 0)
         gpm_report(GPM_PR_OOPS, GPM_MESS_OPEN, GPM_NODE_FIFO);
   }

   if(option.run_status == GPM_RUN_FORK) { /* else is debugging */
      /* goto background and become a session leader (Stefan Giessler) */  
      switch(fork()) {
         case -1: gpm_report(GPM_PR_OOPS,GPM_MESS_FORK_FAILED);   /* error  */
//         case  0: option.run_status = GPM_RUN_DAEMON; break;      /* child  */
         default: _exit(0);                                       /* parent */
      }

      if (setsid() < 0) gpm_report(GPM_PR_OOPS,GPM_MESS_SETSID_FAILED);
   }
   
   /* now we are a daemon */
   option.run_status = GPM_RUN_DAEMON;

   /* damon init: check whether we run or not, display message */
   check_uniqueness();
   gpm_report(GPM_PR_INFO,GPM_MESS_STARTED);

   /* is changing to root needed, because of relative paths ? or can we just
    * remove and ignore it ?? FIXME */
   if (chdir("/") < 0) gpm_report(GPM_PR_OOPS,GPM_MESS_CHDIR_FAILED);
   
   atexit(gpm_exited);                          /* call gpm_exited at the end */
}

/* itz Sat Sep 12 10:30:05 PDT 1998 this function used to mix two
   completely different things; opening a socket to a running daemon
   and checking that a running daemon existed.  Ugly. */
/* rewritten mostly on 20th of February 2002 - nico */
void check_uniqueness(void)
{
   FILE *fp    =  0;
   int old_pid = -1;

   if ((fp = fopen(GPM_NODE_PID, "r")) != NULL) {
      fscanf(fp, "%d", &old_pid);
      if (kill(old_pid, 0) == -1) {
         gpm_report(GPM_PR_INFO,GPM_MESS_STALE_PID, GPM_NODE_PID);
         unlink(GPM_NODE_PID);
      } else /* we are really running, exit asap! */
         gpm_report(GPM_PR_OOPS, GPM_MESS_ALREADY_RUN, old_pid);
   }
   /* now try to sign ourself */
   if ((fp = fopen(GPM_NODE_PID,"w")) != NULL) {
      fprintf(fp,"%d\n",getpid());
      fclose(fp);
   } else {
      gpm_report(GPM_PR_OOPS,GPM_MESS_NOTWRITE,GPM_NODE_PID);
   }
}

/* itz Sat Sep 12 10:55:51 PDT 1998 Added this as replacement for the
   unwanted functionality in check_uniqueness. */
void kill_gpm(void)
{
   int old_pid;
   FILE* fp = fopen(GPM_NODE_PID, "r");

   /* if we cannot find the old pid file, leave */
   if (fp == NULL) gpm_report(GPM_PR_OOPS,GPM_MESS_OPEN, GPM_NODE_PID);

   /* else read the pid */
   if (fscanf(fp, "%d", &old_pid) != 1)
      gpm_report(GPM_PR_OOPS, GPM_MESS_READ_PROB, GPM_NODE_PID);
   fclose(fp);

   gpm_report(GPM_PR_DEBUG, GPM_MESS_KILLING, old_pid);

   /* first check if we run */
   if (kill(old_pid,0) == -1) {
      gpm_report(GPM_PR_INFO, GPM_MESS_STALE_PID, GPM_NODE_PID);
      unlink(GPM_NODE_PID);
   }
   /* then kill us (not directly, but the other instance ... ) */
   if (kill(old_pid, SIGTERM) == -1)
      gpm_report(GPM_PR_OOPS, GPM_MESS_CANT_KILL, old_pid);

   gpm_report(GPM_PR_INFO, GPM_MESS_KILLED, old_pid);
   exit(0);
}

