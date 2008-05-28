/*
 * special commands support for gpm
 *
 * Copyright 1996       Alessandro Rubini <rubini@linux.it>
 * Copyright 1998       Ian Zimmerman     <itz@rahul.net>
 * Copyright 2001-2008  Nico Schottelius <nico-gpm2008 at schottelius.org>
 *
 * Based on an idea by KARSTEN@piobelix.physik.uni-karlsruhe.de
 * (Karsten Ballueder)
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

/* This file is compiled conditionally, see the Makefile */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <sys/param.h>

#include "headers/gpmInt.h"
#include "headers/daemon.h"         /* daemon internals */


/*
 * This function is only called at button press, to avoid unnecessary
 * overhead due to function call at every mouse event
 */

static int special_status = 0; /* turns on when active */
static int did_parse = 0;

  /*
   * The actions are described by these strings. The default is:
   *   left: kill -2 init
   *   middle: shutdown -h now
   *   right: shutdown -r now
   *
   * such a default can be overridden by the argument of the "-S" command.
   * This arg is a colon-separated list of commands. An empty command
   * is used to make gpm kill init
   */

static char *commandL=NULL; /* kill init */
static char *commandM="shutdown -h now";
static char *commandR="shutdown -r now";

/*
 * The return value is 0 if the event has been eaten,
 * 1 if the event is passed on
 */
int processSpecial(Gpm_Event *event)
{
  char *command=NULL; int i;
  FILE *consolef;

  if ((event->type & GPM_TRIPLE)
      && (event->buttons == (GPM_B_LEFT|GPM_B_RIGHT))) /* trigger */
    special_status=time(NULL);

  if (!special_status) /* not triggered: return */
    return 1;

  /* devfs change */
  consolef=fopen(option.consolename,"w");
  if (!consolef) consolef=stderr;
  if (event->type & GPM_TRIPLE) /* just triggered: make noise and return */
    {
    if (!did_parse)
      {
      did_parse++;
      if (opt_special && opt_special[0]) /* not empty */
	{
	commandL = opt_special;
	commandM = strchr(opt_special, ':');
	if (commandM)
	  {
	  *commandM='\0'; commandM++;
	  commandR = strchr(commandM, ':');
	  if (commandR)
	    { *commandR='\0'; commandR++; }
	  }
	}
      }
    fprintf(consolef,"\n%s: release all the mouse buttons and press "
	    "one of them\n\twithin three seconds in order to invoke "
	    "a special command\a\a\a\n", option.progname);
#ifdef DEBUG
    fprintf(consolef,"gpm special: the commands are \"%s\", \"%s\", \"%s\"\n",
	    commandL, commandM, commandR);
#endif
    if (consolef!=stderr) fclose(consolef);
    return 0; /* eaten */
    }

  if (time(NULL) > special_status+3)
    {
    fprintf(consolef,"\n%s: timeout: no special command taken\n", option.progname);
    if (consolef!=stderr) fclose(consolef);
    special_status=0;
    return 0; /* eaten -- don't paste or such on this event */
    }
  special_status=0; /* run now, prevent running next time */
#ifdef DEBUG
  fprintf(consolef,"going to run: buttons is %i\n",event->buttons);
#endif
  switch(event->buttons)
    {
    case GPM_B_LEFT:   command=commandL; break;
    case GPM_B_MIDDLE: command=commandM; break;
    case GPM_B_RIGHT:  command=commandR; break;
    default:           
        fprintf(consolef,"\n%s: more than one button: "
		"special command discarded\n",option.progname);
        if (consolef!=stderr) fclose(consolef);
        return 0; /* eaten */
    }
  fprintf(consolef,"\n%s: executing ", option.progname);

  if (!command || !command[0])
    {
    fprintf(consolef,"hard reboot (by signalling init)\n");
    if (consolef!=stderr) fclose(consolef);
    kill(1,2); /* kill init: shutdown now */
    return 0;
    }

  fprintf(consolef,"\"%s\"\n",command);
  if (consolef!=stderr) fclose(consolef);

  switch(fork())
    {
    case -1: /* error */
      fprintf(stderr,"%s: fork(): %s\n", option.progname, strerror(errno));
      return 0; /* Hmmm.... error */
    
    case 0: /* child */
      close(0); close(1); close(2);
      open(GPM_NULL_DEV,O_RDONLY); /* stdin  */
      open(option.consolename,O_WRONLY); /* stdout */
      dup(1);                     /* stderr */
      int open_max = sysconf(_SC_OPEN_MAX);
      if (open_max == -1) open_max = 1024;
      for (i=3;i<open_max; i++) close(i);
      execl("/bin/sh","sh","-c",command,(char *)NULL);
      exit(1); /* shouldn't happen */
      
    default: /* parent */
      return 0;	
    }
}
