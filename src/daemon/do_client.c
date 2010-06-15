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

#include <unistd.h>                 /* write             */

#include "headers/message.h"        /* messaging in gpm */
#include "headers/daemon.h"         /* daemon internals */
#include "headers/gpmInt.h"         /* daemon internals */

/*-------------------------------------------------------------------*/
/* returns 0 if the event has not been processed, and 1 if it has */
int do_client(Gpm_Cinfo *cinfo, Gpm_Event *event)
{
   Gpm_Connect info=cinfo->data;
   int fd=cinfo->fd;
   /* value to return if event is not used */
   int res = !(info.defaultMask & event->type);

   /* instead of returning 0, scan the stack of clients */
   if ((info.minMod & event->modifiers) < info.minMod)
      goto scan;
   if ((info.maxMod & event->modifiers) < event->modifiers)
      goto scan;

   /* if not managed, use default mask */
   if (!(info.eventMask & GPM_BARE_EVENTS(event->type))) {
      if (res) return res;
      else goto scan;
   }

   /* WARNING */ /* This can generate a SIGPIPE... I'd better catch it */
   MAGIC_P((write(fd,&magic, sizeof(int))));
   write(fd,event, sizeof(Gpm_Event));

   return info.defaultMask & GPM_HARD ? res : 1; /* HARD forces pass-on */

   scan:
   if (cinfo->next != 0)
      return do_client (cinfo->next, event); /* try the next */
   return 0; /* no next, not used */
}

