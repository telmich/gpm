/*
 * libhigh.c - client library - high level (gpm)
 *
 * Copyright 1994,1995   rubini@linux.it (Alessandro Rubini)
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

#include "headers/gpmInt.h"

Gpm_Roi *gpm_roi=NULL;
Gpm_Roi *gpm_current_roi=NULL;
Gpm_Handler *gpm_roi_handler=NULL;
void *gpm_roi_data;


/*-------------------------------------------------------------------*/
Gpm_Roi *Gpm_PushRoi(int x, int y, int X, int Y, int mask,
					 Gpm_Handler *fun, void *xtradata)
{
Gpm_Roi *new;

  /* create a Roi and push it */
  new=(Gpm_Roi *)malloc(sizeof(Gpm_Roi));
  if (!new) return NULL;

  /* use the roi handler, if still null */
  if (!gpm_roi && !gpm_handler)
	gpm_handler=Gpm_HandleRoi;

  new->xMin=x;         new->xMax=X;
  new->yMin=y;         new->yMax=Y;
  new->minMod=0;       new->maxMod=~0;
  new->prev      =     new->next=NULL;
  new->eventMask=mask;
  new->owned=0;        /* use free() */
  new->handler=fun;
  new->clientdata = xtradata ? xtradata : (void *)new;
  return Gpm_RaiseRoi(new,NULL);
}

/*-------------------------------------------------------------------*/
Gpm_Roi *Gpm_UseRoi(Gpm_Roi *new)
{
  /* use a Roi by pushing it */
  new->prev      =     new->next=NULL;
  new->owned=1;        /* don't free() */

  /* use the roi handler, if still null */
  if (!gpm_roi && !gpm_handler)
	gpm_handler=Gpm_HandleRoi;

  return Gpm_RaiseRoi(new,NULL);
}

/*-------------------------------------------------------------------*/
Gpm_Roi *Gpm_PopRoi(Gpm_Roi *which)
{
  /* extract the Roi and remove it */
  if (which->prev) which->prev->next = which->next;
  if (which->next) which->next->prev = which->prev;
  if (gpm_roi==which) gpm_roi=which->next;
  
  if (which->owned==0) free(which);
  if (gpm_current_roi==which)
	gpm_current_roi=NULL;
  
  return gpm_roi; /* return the new top-of-stack */
}

/*-------------------------------------------------------------------*/
Gpm_Roi *Gpm_RaiseRoi(Gpm_Roi *which, Gpm_Roi *before)
{
  /* raise a Roi above another, or to top-of-stack */
  if (!gpm_roi)
	return gpm_roi=which;
  if (!before) before=gpm_roi;
  if (before==which) return gpm_roi;

  if (which->prev) which->prev->next = which->next;
  if (which->next) which->next->prev = which->prev;
  if (gpm_roi==which) gpm_roi=which->next;

  which->prev=before->prev; before->prev=which;
  which->next=before;

  if (which->prev) which->prev->next=which;
  else gpm_roi=which;

  return gpm_roi; /* return the new top-of-stack */
}

/*-------------------------------------------------------------------*/
Gpm_Roi *Gpm_LowerRoi(Gpm_Roi *which, Gpm_Roi *after)
{
  /* lower a Roi below another, or to bottom-of-stack */
  if (!after)
    for (after=gpm_roi; after->next; after=after->next)
      ;
  if (after==which) return gpm_roi;
  if (which->prev) which->prev->next = which->next;
  if (which->next) which->next->prev = which->prev;
  if (gpm_roi==which) gpm_roi=which->next;

  which->next=after->next; after->next=which;
  which->prev=after;

  if (which->next) which->next->prev=which;

  return gpm_roi; /* return the new top-of-stack */
}



/*===================================================================*
 * This function is in care of all event dispatching.
 * It generates also GPM_ENTER and GPM_LEAVE events.
 */

int Gpm_HandleRoi(Gpm_Event *ePtr, void *clientdata)
{
static Gpm_Event backEvent;
Gpm_Roi *roi=gpm_current_roi;

/*
 * If motion or press, look for the interested roi.
 * Drag and release will be reported to the old roi.
 */

  if (ePtr->type&(GPM_MOVE|GPM_DOWN))
	for (roi=gpm_roi; roi; roi=roi->next)
	  {
	  if (roi->xMin > ePtr->x || roi->xMax < ePtr->x)
		continue;
	  if (roi->yMin > ePtr->y || roi->yMax < ePtr->y)
		continue;
	  if ((roi->minMod & ePtr->modifiers) < roi->minMod) continue;
	  if ((roi->maxMod & ePtr->modifiers) < ePtr->modifiers) continue;
	  break;
	  }

/*
 * Now generate the leave/enter events
 */

  if (roi!=gpm_current_roi)
	{
	if (gpm_current_roi && (gpm_current_roi->eventMask & GPM_LEAVE))
	  {
	  backEvent.type=GPM_LEAVE;
	  (*(gpm_current_roi->handler))(&backEvent,gpm_current_roi->clientdata);
	  }
	if (roi && (roi->eventMask & GPM_ENTER))
	  {
	  backEvent.type=GPM_ENTER;
	  (*(roi->handler))(&backEvent,roi->clientdata);
	  }
	}
  gpm_current_roi=roi;

  /*
   * events not requested are discarded
   */
  if (roi && (GPM_BARE_EVENTS(ePtr->type) & roi->eventMask) == 0)
	return 0; 

  backEvent=*ePtr; /* copy it, so the main one is unchanged */
  if (!roi)
	{
	if (gpm_roi_handler)
	  return (*gpm_roi_handler)(&backEvent,gpm_roi_data);
	return 0;
	}


/*
 * Ok, now report the event as it is, after modifying x and y
 */

  backEvent.x -= roi->xMin;
  backEvent.y -= roi->yMin;
  return (*(roi->handler))(&backEvent,roi->clientdata);
}

