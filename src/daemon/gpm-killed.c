/*
 * general purpose mouse support
 *
 * Copyright (C) 1993        Andreq Haylett <ajh@gec-mrc.co.uk>
 * Copyright (C) 1994-1999   Alessandro Rubini <rubini@linux.it>
 * Copyright (C) 1998        Ian Zimmerman <itz@rahul.net>
 * Copyright (c) 2001-2008   Nico Schottelius <nico-gpm2008 at schottelius.org>
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

#include <signal.h>                 /* SIG*              */
#include <stdlib.h>                 /* exit()            */
#include <unistd.h>                 /* getpid()          */

#include "headers/message.h"        /* messaging in gpm */
#include "headers/daemon.h"         /* daemon internals */

void gpm_killed(int signo)
{
   if(signo == SIGWINCH) {
      gpm_report(GPM_PR_WARN, GPM_MESS_RESIZING, option.progname, getpid());
      opt_resize++;
      return;
   }
   if(signo==SIGUSR1) {
     gpm_report(GPM_PR_WARN,GPM_MESS_KILLED_BY, option.progname, getpid(), option.progname);
   }

   exit(0);
}
