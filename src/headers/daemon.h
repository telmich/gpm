/*
 * Daemon internals
 *
 * Copyright (c) 2008    Nico Schottelius <nico-gpm2008 at schottelius.org>
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
 */

#ifndef _GPM_DAEMON_H
#define _GPM_DAEMON_H

/*************************************************************************
 * Types
 */

struct options {
   int autodetect;            /* -u [aUtodetect..'A' is not available] */
   int no_mice;               /* number of mice */
   int repeater;              /* repeat data */
   char *repeater_type;       /* repeat data as which mouse type */
   int run_status;            /* startup/daemon/debug */
   char *progname;            /* hopefully gpm ;) */
   struct micetab *micelist;  /* mice and their options */
   char *consolename;         /* /dev/tty0 || /dev/vc/0 */
};

/*************************************************************************
 * Global variables
 */

extern int              opt_resize;       /* not really an option          */
extern struct options   option;           /* one should be enough for us   */
extern int              mouse_argc[3];    /* 0 for default (unused)        */
extern char           **mouse_argv[3];    /* and two mice                  */
extern int              opt_aged;
extern int        statusX,statusY,statusB;


/*************************************************************************
 * Functions
 */

void gpm_killed(int signo);
int open_console(const int mode);
char **build_argv(char *argv0, char *str, int *argcptr, char sep);
int wait_text(int *fdptr);

void selection_copy(int x1, int y1, int x2, int y2, int mode);
void selection_paste(void);


#endif
