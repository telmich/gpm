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

#include <errno.h>

#include "message.h"       /* messaging in gpm */
#include "mice.h"          /* daemon internals */
#include "daemon.h"        /* option           */

/*========================================================================*/
/* Parse the "old" -o options */
/*========================================================================*/
int option_modem_lines(int fd, int argc, char **argv)
{
   static unsigned int err, lines, reallines;

   static argv_helper optioninfo[] = {
      {"dtr",  ARGV_BOOL, u: {iptr: &lines}, value: TIOCM_DTR},
      {"rts",  ARGV_BOOL, u: {iptr: &lines}, value: TIOCM_RTS},
      {"both", ARGV_BOOL, u: {iptr: &lines}, value: TIOCM_DTR | TIOCM_RTS},
      {"",       ARGV_END}
      };

   if (argc<2) return 0;
   if (argc > 2) {
      gpm_report(GPM_PR_ERR,GPM_MESS_TOO_MANY_OPTS,option.progname, argv[0]);
      errno = EINVAL; /* used by gpm_oops(), if the caller reports failure */
      return -1;
   }
   err = parse_argv(optioninfo, argc, argv);
   if(err) return 0; /* a message has been printed, but go on as good */

   /* ok, move the lines */
   ioctl(fd, TIOCMGET, &reallines);
   reallines &= ~lines;
   ioctl(fd, TIOCMSET, &reallines);
   return 0;
}

