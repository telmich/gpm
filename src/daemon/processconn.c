
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

#include <sys/socket.h>         /* accept */
#include <stdlib.h>             /* malloc */
#include <unistd.h>             /* close */
#include <sys/un.h>             /* unix socket */
#include <sys/stat.h>           /* stat */
#include <string.h>             /* str* */
#include <errno.h>              /* errno.h */

#include "headers/message.h"    /* messaging in gpm */
#include "headers/daemon.h"     /* daemon internals */

int processConn(int fd)
{                               /* returns newfd or -1 */
   Gpm_Cinfo *info;
   Gpm_Connect *request;
   Gpm_Cinfo *next;
   int vc, newfd;
   socklen_t len;
   struct sockaddr_un addr;     /* reuse this each time */
   struct stat statbuf;
   uid_t uid;
   char *tty = NULL;

/*....................................... Accept */

   bzero((char *) &addr, sizeof(addr));
   addr.sun_family = AF_UNIX;

   len = sizeof(addr);
   if((newfd = accept(fd, (struct sockaddr *) &addr, &len)) == -1) {
      gpm_report(GPM_PR_ERR, GPM_MESS_ACCEPT_FAILED, strerror(errno));
      return -1;
   }

   gpm_report(GPM_PR_DEBUG, GPM_MESS_CONECT_AT, newfd);

   info = malloc(sizeof(Gpm_Cinfo));
   if(!info)
      gpm_report(GPM_PR_OOPS, GPM_MESS_NO_MEM);
   request = &(info->data);

   if(get_data(request, newfd) == -1) {
      free(info);
      close(newfd);
      return -1;
   }

   if((vc = request->vc) > MAX_VC) {
      gpm_report(GPM_PR_DEBUG, GPM_MESS_REQUEST_ON, vc, MAX_VC);
      free(info);
      close(newfd);
      return -1;
   }
#ifndef SO_PEERCRED
   if(stat(addr.sun_path, &statbuf) == -1 || !S_ISSOCK(statbuf.st_mode)) {
      gpm_report(GPM_PR_ERR, GPM_MESS_ADDRES_NSOCKET, addr.sun_path);
      free(info);               /* itz 10-12-95 verify client's right */
      close(newfd);
      return -1;                /* to read requested tty */
   }

   unlink(addr.sun_path);       /* delete socket */

   staletime = time(0) - 30;
   if(statbuf.st_atime < staletime
      || statbuf.st_ctime < staletime || statbuf.st_mtime < staletime) {
      gpm_report(GPM_PR_ERR, GPM_MESS_SOCKET_OLD);
      free(info);
      close(newfd);
      return -1;                /* socket is ancient */
   }

   uid = statbuf.st_uid;        /* owner of socket */
#else
   {
      struct ucred sucred;
      socklen_t credlen = sizeof(struct ucred);

      if(getsockopt(newfd, SOL_SOCKET, SO_PEERCRED, &sucred, &credlen) == -1) {
         gpm_report(GPM_PR_ERR, GPM_MESS_GETSOCKOPT, strerror(errno));
         free(info);
         close(newfd);
         return -1;
      }
      uid = sucred.uid;
      gpm_report(GPM_PR_DEBUG, GPM_MESS_PEER_SCK_UID, uid);
   }
#endif
   if(uid != 0) {
      if((tty =
          malloc(strlen(option.consolename) + Gpm_cnt_digits(vc) +
                 sizeof(char))) == NULL)
         gpm_report(GPM_PR_OOPS, GPM_MESS_NO_MEM);

      strncpy(tty, option.consolename, strlen(option.consolename) - 1);
      sprintf(&tty[strlen(option.consolename) - 1], "%d", vc);

      if(stat(tty, &statbuf) == -1) {
         gpm_report(GPM_PR_ERR, GPM_MESS_STAT_FAILS, tty);
         free(info);
         free(tty);
         close(newfd);
         return -1;
      }
      if(uid != statbuf.st_uid) {
         gpm_report(GPM_PR_WARN, GPM_MESS_FAILED_CONNECT, uid, tty);    /* SUSPECT! */
         free(info);
         free(tty);
         close(newfd);
         return -1;
      }
      free(tty);                /* at least here it's not needed anymore */
   }

   /*
    * register the connection information in the right place 
    */
   info->next = next = cinfo[vc];
   info->fd = newfd;
   cinfo[vc] = info;
   gpm_report(GPM_PR_DEBUG, GPM_MESS_LONG_STATUS,
              request->pid, request->vc, request->eventMask,
              request->defaultMask, request->minMod, request->maxMod);

   /*
    * if the client gets motions, give it the current position 
    */
   if(request->eventMask & GPM_MOVE) {
      Gpm_Event event = { 0, 0, vc, 0, 0, statusX, statusY, GPM_MOVE, 0, 0 };
      do_client(info, &event);
   }

   return newfd;
}
