/*
 * general purpose mouse support for Linux
 *
 * *several tools only needed by the server*
 *
 * Copyright (c) 2002-2008    Nico Schottelius <nico-gpm2008 at schottelius.org>
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

#include "headers/gpmInt.h"
#include "headers/message.h"
#include "headers/daemon.h"

#include <stdlib.h> /* malloc() */

/* DESCR:   mice initialization. currently print mice. */
/* RETURN:  0 - failed to init one or more devices 
            1 - init was fine */
/* COMMENT: does error handling and exiting itself */
int init_mice(struct micetab *micelist)
{
   struct micetab *tmp = micelist;
   
   while(tmp != NULL) {           /* there are still mice to init */
      gpm_report(GPM_PR_DEBUG,"initialize %s with proto %s",tmp->device,tmp->protocol);
      if(tmp->options != NULL) {
         gpm_report(GPM_PR_DEBUG,"and options %s",tmp->options);
      }
      tmp = tmp->next;
   }

   gpm_report(GPM_PR_DEBUG,"finished initialization");
   return 1;
}
