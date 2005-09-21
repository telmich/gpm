/*
 * general purpose mouse support for Linux
 *
 * *several tools only needed by the server*
 *
 * Copyright (c) 2002         Nico Schottelius <nico-gpm@schottelius.org>
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

#include <string.h>
#include <stdlib.h> /* malloc() */
#include <sys/fcntl.h>

#include "headers/gpmInt.h"
#include "headers/message.h"

struct micetab *micelist;

/* DESCR:   allocate a new mouse and to the list of mice. initialization follows later */
/* RETURN:  new mouse structure */
/* COMMENT: does error handling and exiting itself */
struct micetab *add_mouse(void)
{
   struct micetab *mouse;

   gpm_report(GPM_PR_DEBUG, "adding mouse device");
   if (!(mouse = malloc(sizeof(struct micetab))))
      gpm_report(GPM_PR_OOPS, GPM_MESS_NO_MEM);

   memset(mouse, 0, sizeof(struct micetab));

   mouse->dev.timeout = -1;

   mouse->options.sequence = NULL;
   mouse->options.sample = DEF_SAMPLE;
   mouse->options.delta = DEF_DELTA;
   mouse->options.accel = DEF_ACCEL;
   mouse->options.scalex = DEF_SCALE;
   mouse->options.scaley = DEF_SCALE;
   mouse->options.time = DEF_TIME;
   mouse->options.cluster = DEF_CLUSTER;
   mouse->options.three_button = DEF_THREE;
   mouse->options.glidepoint_tap = DEF_GLIDEPOINT_TAP;
   mouse->options.text = NULL;

   mouse->next = micelist;
   micelist = mouse;

   return mouse;
}

/* DESCR:   mice initialization. calls appropriate init functions. */
/* COMMENT: does error handling and exiting itself */
void init_mice(void)
{
   struct micetab *mouse;

   for (mouse = micelist; mouse; mouse = mouse->next) {
      if (!strcmp(mouse->device, "-"))
         mouse->dev.fd = 0; /* use stdin */
      else if ((mouse->dev.fd = open(mouse->device, O_RDWR | O_NDELAY)) < 0)
         gpm_report(GPM_PR_OOPS, GPM_MESS_OPEN, mouse->device);
   
      /* and then reset the flag */
      fcntl(mouse->dev.fd, F_SETFL, fcntl(mouse->dev.fd, F_GETFL) & ~O_NDELAY);

      /* init the device, and use the return value as new mouse type */
      if (mouse->type->init)
         if (mouse->type->init(&mouse->dev, &mouse->options, mouse->type))
            gpm_report(GPM_PR_OOPS, GPM_MESS_MOUSE_INIT);
   }
}

/* DESCR:   when leaving, we should reset mice to their normal state */
/* COMMENT: does error handling and exiting itself */
void cleanup_mice(void)
{
   struct micetab *tmp;

   while ((tmp = micelist)) {
      if (micelist->dev.private)
         free(micelist->dev.private);
      micelist = micelist->next;
      free(tmp);
   }   
}
