
/*
 * gpm2 - mouse driver for the console
 *
 * Copyright (c) 2007-2008 Nico Schottelius <nico-gpm2 () schottelius.org>
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
 *
 *    Become a daemon
 ********/

#include <fcntl.h>              /* open */
#include <unistd.h>             /* close/dup2 */
#include <stdio.h>              /* perror */

#include "gpm2-messages.h"      /* GPM2_MESS_* */

#define NULL_DEV "/dev/null"

int become_daemon()
{
   int fd;

   /*
    * fork 
    */
   switch (fork()) {
      case -1:                 /* error */
         perror(GPM2_MESS_FORK);
         return 0;
         break;
      case 0:                  /* child */
         break;
      default:
         _exit(0);              /* parent */
   }

   /***************** child code ******************/

   /*
    * change fds 
    */
   fd = open(NULL_DEV, O_RDWR);
   if(fd == -1) {
      perror(GPM2_MESS_OPEN);
      return 0;
   }

   if(setsid() == -1) {
      perror(GPM2_MESS_SETSID);
      return 0;
   }
   if(chdir("/") == -1) {
      perror(GPM2_MESS_CHDIR);
      return 0;
   }

   /*
    * FIXME perhaps connect to syslog? 
    */
   if(dup2(fd, STDIN_FILENO) == -1 ||
      dup2(fd, STDOUT_FILENO) == -1 || dup2(fd, STDERR_FILENO) == -1) {
      perror(GPM2_MESS_DUP2);
      return 0;
   }

   if(close(fd) == -1) {
      /*
       * makes no sense, as stderr is dead. use gpm2_report later
       * perror(GPM2_MESS_CLOSE); 
       */
      return 0;
   }

   return 1;
}
