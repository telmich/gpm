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

/* DESCR:   when leaving, we should reset mice to their normal state */
/* RETURN:  0 - failed to reset one or more devices 
            1 - reset was fine */
/* COMMENT: does error handling and exiting itself */
int reset_mice(struct micetab *micelist)
{
   struct micetab *tmp = micelist;
   struct micetab *end = tmp;

   while(tmp != NULL) { /* FIXME! I never get NULL, as free()d before */
      end = tmp;
      while(tmp->next != NULL) {       /* set end to the last mouse */
         end = tmp;
         tmp = tmp->next;
      }

      gpm_report(GPM_PR_DEBUG,"reset: %s with proto %s",end->device,end->protocol);
      if(tmp->options != NULL) {
         gpm_report(GPM_PR_DEBUG,"and options %s",end->options);
      }
      free(end);                       /* be clean() */
      tmp = micelist;                  /* reset to the first mice again */
   }   

   return 1;
}
