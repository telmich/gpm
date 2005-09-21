/*
 * libxtra.c - client library - extra functions (gpm-Linux)
 *
 * Copyright 1994,1995   rubini@linux.it (Alessandro Rubini)
 * Copyright (C) 1998    Ian Zimmerman <itz@rahul.net>
 * 
 * xterm management is mostly by Miguel de Icaza
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
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include "headers/gpmInt.h"
#include "headers/message.h"


/*-------------------------------------------------------------------*/

/*
 * these two functions return version information 
 */

static char *gpml_ver_s=GPM_RELEASE;
static int  gpml_ver_i = 0;

char *Gpm_GetLibVersion(int *where)
{
  int i,j,k=0;

  if (!gpml_ver_i)
    {
      sscanf(gpml_ver_s,"%d.%d.%d",&i,&j,&k);
      gpml_ver_i=i*10000+j*100+k;
    }
  if (where) *where=gpml_ver_i;
  return gpml_ver_s;
}

static char gpm_ver_s[16];
static int  gpm_ver_i = 0;

char *Gpm_GetServerVersion(int *where)
{
  char line[128];
  FILE *f;
  int i,j,k=0;

  if (!gpm_ver_s[0])
    {
      f=popen(SBINDIR "/gpm -v","r");
      if (!f) return NULL;
      fgets(line,128,f);
      if (pclose(f)) return 0;
      sscanf(line,"%*s %s",gpm_ver_s);  /* "gpm-Linux 0.98, March 1995" */
      gpm_ver_s[strlen(gpm_ver_s)-1]='\0'; /* cut the ',' */

      sscanf(gpm_ver_s,"%d.%d.%d",&i,&j,&k);
      gpm_ver_i=i*10000+j*100+k;
    }
  
  if (where) *where=gpm_ver_i;
  return gpm_ver_s;
}

/*-------------------------------------------------------------------*/
/*
 * This returns all the available information about the current situation:
 * The return value is the number of buttons, as known to the server,
 * the ePtr, if any, is filled with information on the current state.
 */
int Gpm_GetSnapshot(Gpm_Event *ePtr)
{
  Gpm_Connect conn;
  Gpm_Event event;
  fd_set sillySet;
  struct timeval to={0,0};
  int i;

  if (!gpm_ver_i) {
      if (0 == Gpm_GetServerVersion(NULL)) {
        gpm_report(GPM_PR_WARN,"can't get gpm server version");
      } /*if*/
      gpm_report(GPM_PR_INFO,"libgpm: got server version as %i",gpm_ver_i);
    }
  if (gpm_ver_i<9802) {
      gpm_report(GPM_PR_INFO,"gpm server version too old to obtain status info");
      return -1; /* error */
    }
  if (gpm_fd <=0)
    {
      gpm_report(GPM_PR_INFO,"gpm connection must be open to obtain status info");
      return -1; /* error */
    }

  conn.pid=0; /* this signals a request */
  if (ePtr)
    conn.vc=GPM_REQ_SNAPSHOT;
  else
    {
      conn.vc=GPM_REQ_BUTTONS;
      ePtr=&event;
    }

  if (gpm_fd==-1) return -1;
  FD_ZERO(&sillySet);
  FD_SET(gpm_fd,&sillySet);
  if (select(gpm_fd+1,&sillySet,NULL,NULL,&to)==1)
    return 0;
  write(gpm_fd,&conn,sizeof(Gpm_Connect));

  if ((i=Gpm_GetEvent(ePtr))!=1)
    return -1;

  i=ePtr->type; ePtr->type=0;
  return i; /* number of buttons */
}
  

/* Local Variables: */
/* c-indent-level: 2 */
/* End: */
