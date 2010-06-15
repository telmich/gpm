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

/* DESCR:   add this to the list of mice. initialization follows later */
/* RETURN:  - */
/* COMMENT: does error handling and exiting itself */
void add_mouse(int type, char *value)
{
   struct micetab *tmp = option.micelist;

   /* PREAMBLE for all work: */
   /* -m /dev/misc/psaux -t ps2 [ -o options ] */

   switch(type) {

      /*---------------------------------------------------------------------*/
      /********************** -m mousedevice *********************************/
      /*---------------------------------------------------------------------*/

      case GPM_ADD_DEVICE:

         /* first invocation */
         if(option.micelist == NULL) {
            gpm_report(GPM_PR_DEBUG,"adding mouse device: %s",value);
            option.micelist = (struct micetab *) malloc(sizeof(struct micetab));
            if(!option.micelist) gpm_report(GPM_PR_OOPS,GPM_MESS_NO_MEM);
            option.micelist->next      = NULL;
            option.micelist->device    = value;
            option.micelist->protocol  = NULL;
            option.micelist->options   = NULL;
            return;
         }
         
         /* find actual mouse */
         while(tmp->device != NULL && tmp->protocol != NULL && tmp->next !=NULL)
            tmp = tmp->next;

         gpm_report(GPM_PR_DEBUG,"finished searching");

         /* found end of micelist, add new mouse */
         if(tmp->next == NULL && tmp->protocol != NULL) {
            gpm_report(GPM_PR_DEBUG,"next mouse making");
            tmp->next = (struct micetab *) malloc(sizeof(struct micetab));
            if(!tmp) gpm_report(GPM_PR_OOPS,GPM_MESS_NO_MEM);
            tmp->next      = NULL;
            tmp->device    = value;
            tmp->protocol  = NULL;
            tmp->options   = NULL;
            return;
         } else gpm_report(GPM_PR_OOPS,GPM_MESS_FIRST_DEV);
         
         //} else if(tmp->device != NULL && tmp->protocol == NULL)
         // gpm_report(GPM_PR_OOPS,GPM_MESS_FIRST_DEV); /* -m -m */

         
         break;

      /*---------------------------------------------------------------------*/
      /************************* -t type / protocol **************************/
      /*---------------------------------------------------------------------*/

      case GPM_ADD_TYPE:
         if(option.micelist == NULL) gpm_report(GPM_PR_OOPS,GPM_MESS_FIRST_DEV);
         
         /* skip to next mouse, where either device or protocol is missing */
         while(tmp->device != NULL && tmp->protocol != NULL && tmp->next !=NULL)
            tmp = tmp->next;
         
         /* check whether device (-m) is there, if so, write protocol */
         if(tmp->device == NULL) gpm_report(GPM_PR_OOPS,GPM_MESS_FIRST_DEV);
         else {
            gpm_report(GPM_PR_DEBUG,"adding mouse type: %s",value);
            tmp->protocol = value;
            option.no_mice++;          /* finally we got our mouse */
         }

         break;

      /*---------------------------------------------------------------------*/
      /*************************** -o options ********************************/
      /*---------------------------------------------------------------------*/

      case GPM_ADD_OPTIONS:
         if(option.micelist == NULL) gpm_report(GPM_PR_OOPS,GPM_MESS_FIRST_DEV);

         /* look for the last mouse */
         tmp = option.micelist;
         while(tmp->next != NULL) tmp = tmp->next;

         /* if -m or -t are missing exit */
         if(tmp->device == NULL || tmp->protocol == NULL)
            gpm_report(GPM_PR_OOPS,GPM_MESS_FIRST_DEV);
         else {
            gpm_report(GPM_PR_DEBUG,"adding mouse options: %s",value);
            tmp->options = value;
         }   
         break;
   }
}
