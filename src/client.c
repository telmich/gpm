/*
 * client.c - GPM client handling (server side)
 *
 * Copyright (C) 1993        Andreq Haylett <ajh@gec-mrc.co.uk>
 * Copyright (C) 1994-1999   Alessandro Rubini <rubini@linux.it>
 * Copyright (C) 1998        Ian Zimmerman <itz@rahul.net>
 * Copyright (c) 2001,2002   Nico Schottelius <nico-gpm@schottelius.org>
 * Copyright (C) 2003        Dmitry Torokhov <dtor@mail.ru>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>        /* strerror(); ?!?  */
#include <errno.h>
#include <unistd.h>        /* select(); */
#include <signal.h>        /* SIGPIPE */
#include <time.h>          /* time() */
#include <sys/fcntl.h>     /* O_RDONLY */
#include <sys/stat.h>      /* mkdir()  */
#include <sys/time.h>      /* timeval */
#include <sys/types.h>     /* socket() */
#include <sys/socket.h>    /* socket() */
#include <sys/un.h>        /* struct sockaddr_un */

#include "headers/gpmInt.h"
#include "headers/message.h"
#include "headers/console.h"
#include "headers/selection.h"
#include "headers/client.h"

/* who the f*** runs gpm without glibc? */
#if !defined(__GLIBC__)
   typedef unsigned int __socklen_t;
#endif    /* __GLIBC__ */

#ifndef max
#define max(a,b) ((a)>(b) ? (a) : (b))
#endif

extern int errno;

struct client_info *cinfo[MAX_VC + 1];

/*-------------------------------------------------------------------*
 * This was inline, and incurred in a compiler bug (2.7.0)
 *-------------------------------------------------------------------*/
static int get_data(int fd, Gpm_Connect *data)
{
   static int len;

#ifdef GPM_USE_MAGIC
   while ((len = read(whence, &check, sizeof(int))) == 4 &&
          check != GPM_MAGIC)
      gpm_report(GPM_PR_INFO, GPM_MESS_NO_MAGIC);

   if (len == 0) return 0;

   if (check != GPM_MAGIC) {
      gpm_report(GPM_PR_INFO, GPM_MESS_NOTHING_MORE);
      return -1;
   }
#endif

   len = read(fd, data, sizeof(Gpm_Connect));

   return len ? (len == sizeof(Gpm_Connect) ? 1 : -1) : 0;
}

/*-------------------------------------------------------------------*/
int listen_for_clients(void)
{
   struct sockaddr_un ctladdr;
   int fd, len;

   unlink(GPM_NODE_CTL);

   if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
      gpm_report(GPM_PR_OOPS, GPM_MESS_SOCKET_PROB);

   memset(&ctladdr, 0, sizeof(ctladdr));
   ctladdr.sun_family = AF_UNIX;
   strcpy(ctladdr.sun_path, GPM_NODE_CTL);
   len = sizeof(ctladdr.sun_family) + strlen(GPM_NODE_CTL);

   if (bind(fd, (struct sockaddr *)&ctladdr, len) == -1)
      gpm_report(GPM_PR_OOPS, GPM_MESS_BIND_PROB, ctladdr.sun_path);

   /* needs to be 0777, so all users can _try_ to access gpm */
   chmod(GPM_NODE_CTL, 0777);
   listen(fd, 5);          /* Queue up calls */

   return fd;
}

/*-------------------------------------------------------------------*/
struct client_info *accept_client_connection(int fd)
{
   struct client_info *info;
   Gpm_Connect *request;
   int newfd;
#if !defined(__GLIBC__)
   int len;
#else /* __GLIBC__ */
   size_t len; /* isn't that generally defined in C ???  -- nico */
#endif /* __GLIBC__ */
   struct sockaddr_un addr; /* reuse this each time */
#ifndef SO_PEERCRED
   struct stat statbuf;
   time_t staletime;
#endif
   uid_t uid;

   /*....................................... Accept */
   memset(&addr, 0, sizeof(addr));
   addr.sun_family = AF_UNIX;

   len = sizeof(addr);
   if ((newfd = accept(fd, (struct sockaddr *)&addr, &len)) < 0) {
      gpm_report(GPM_PR_ERR, GPM_MESS_ACCEPT_FAILED, strerror(errno));
      return NULL;
   }

   gpm_report(GPM_PR_INFO, GPM_MESS_CONECT_AT, newfd);

   if (!(info = malloc(sizeof(struct client_info))))
      gpm_report(GPM_PR_OOPS, GPM_MESS_NO_MEM);

   request = &info->data;
   if (get_data(newfd, request) == -1)
      goto err;

   if (request->vc > MAX_VC) {
      gpm_report(GPM_PR_WARN,GPM_MESS_REQUEST_ON, request->vc, MAX_VC);
      goto err;
   }

#ifndef SO_PEERCRED
   if (stat(addr.sun_path, &statbuf) == -1 || !S_ISSOCK(statbuf.st_mode)) {
      gpm_report(GPM_PR_ERR,GPM_MESS_ADDRES_NSOCKET,addr.sun_path);
      goto err;
   }

   unlink(addr.sun_path);   /* delete socket */

   staletime = time(0) - 30;
   if (statbuf.st_atime < staletime ||
       statbuf.st_ctime < staletime ||
       statbuf.st_mtime < staletime) {
      gpm_report(GPM_PR_ERR, GPM_MESS_SOCKET_OLD);
      goto err;
   }

   uid = statbuf.st_uid;      /* owner of socket */
#else
   {
      struct ucred sucred;
      socklen_t credlen = sizeof(struct ucred);

      if (getsockopt(newfd, SOL_SOCKET, SO_PEERCRED, &sucred, &credlen) == -1) {
         gpm_report(GPM_PR_ERR,GPM_MESS_GETSOCKOPT, strerror(errno));
         goto err;
      }
      uid = sucred.uid;
      gpm_report(GPM_PR_DEBUG,GPM_MESS_PEER_SCK_UID, uid);
   }
#endif

   if (uid != 0 && !is_console_owner(request->vc, uid)) {
      gpm_report(GPM_PR_WARN, GPM_MESS_FAILED_CONNECT, uid, request->vc);
      goto err;
   }

   /* register the connection information in the right place */
   info->next = cinfo[request->vc];
   info->fd = newfd;
   cinfo[request->vc] = info;
   gpm_report(GPM_PR_DEBUG, GPM_MESS_LONG_STATUS,
        request->pid, request->vc, request->eventMask, request->defaultMask,
        request->minMod, request->maxMod);

   return info;

err:
   free(info);
   close(newfd);

   return NULL;
}

/*-------------------------------------------------------------------*/
void remove_client(struct client_info *ci, int vc)
{
   struct client_info *p, *prev = NULL;

   for (p = cinfo[vc]; p; prev = p, p = p->next) {
      if (p == ci) {
         if (!prev)    /* it is on top of the stack */
            cinfo[vc] = p->next;
         else
            prev->next = p->next;
         break;
      }
   }
   if (p) free(p);
}

/*-------------------------------------------------------------------*/
void notify_clients_resize(void)
{
   struct client_info *ci;
   int i;

   for (i = 0; i < MAX_VC + 1; i++)
      for (ci = cinfo[i]; ci; ci = ci->next)
         kill(ci->data.pid, SIGWINCH);
}

/*-------------------------------------------------------------------*/
/* returns 0 if the event has not been processed, and 1 if it has    */
int do_client(struct client_info *cinfo, Gpm_Event *event)
{
   Gpm_Connect *info = &cinfo->data;
   /* value to return if event is not used */
   int res = !(info->defaultMask & event->type);

   /* instead of returning 0, scan the stack of clients */
   if ((info->minMod & event->modifiers) < info->minMod)
      goto try_next;
   if ((info->maxMod & event->modifiers) < event->modifiers)
      goto try_next;

   /* if not managed, use default mask */
   if (!(info->eventMask & GPM_BARE_EVENTS(event->type))) {
      if (res) return res;
      else goto try_next;
   }

   /* WARNING */ /* This can generate a SIGPIPE... I'd better catch it */
   MAGIC_P((write(cinfo->fd, &magic, sizeof(int))));
   write(cinfo->fd, event, sizeof(Gpm_Event));

   return info->defaultMask & GPM_HARD ? res : 1; /* HARD forces pass-on */

 try_next:
   if (cinfo->next != 0)
      return do_client(cinfo->next, event); /* try the next */

   return 0; /* no next, not used */
}

/*-------------------------------------------------------------------*/
/* returns 0 if client disconnects, -1 - error, 1 -successs */
int process_client_request(struct client_info *ci, int vc,
                           int x, int y, int buttons, int clicks,
                           int three_button_mouse)
{
   int rc;
   Gpm_Connect conn;
   static Gpm_Event event;

   gpm_report(GPM_PR_INFO, GPM_MESS_CON_REQUEST, ci->fd, vc);
   if (vc > MAX_VC) return -1;

   /* itz 10-22-96 this shouldn't happen now */
   if (vc == -1) gpm_report(GPM_PR_OOPS, GPM_MESS_UNKNOWN_FD);

   rc = get_data(ci->fd, &conn);

   if (rc == 0) { /* no data */
      gpm_report(GPM_PR_INFO, GPM_MESS_CLOSE);
      close(ci->fd);
      return 0;
   }

   if (rc == -1) return -1; /* too few bytes */

   if (conn.pid != 0) {
      ci->data = conn;
      return 1;
   }

   /* Aha, request for information (so-called snapshot) */
   switch (conn.vc) {
      case GPM_REQ_SNAPSHOT:
         event.vc = get_console_state(&event.modifiers);
         event.x = x; event.y = y;
         event.buttons = buttons;
         event.clicks = clicks;
         event.dx = console.max_x; event.dy = console.max_y;
         /* fall through */

      case GPM_REQ_BUTTONS:
         event.type = (three_button_mouse == 1 ? 3 : 2); /* buttons */
         write(ci->fd, &event, sizeof(Gpm_Event));
         break;

      case GPM_REQ_NOPASTE:
         selection_disable_paste();
         gpm_report(GPM_PR_INFO, GPM_MESS_DISABLE_PASTE, vc);
         break;
   }

   return 1;
}

