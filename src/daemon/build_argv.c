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

#include <string.h>        /* str*     */
#include <stdlib.h>        /* calloc   */

#include "headers/message.h"        /* messaging in gpm */

/* build_argv is used for mouse initialization routines */
char **build_argv(char *argv0, char *str, int *argcptr, char sep)
{
   int argc = 1;
   char **argv;
   char *s;

   /* argv0 is never NULL, but the extra string may well be */
   if (str)
      for (s=str; sep && (s = strchr(s, sep)); argc++) s++;

   argv = calloc(argc+2, sizeof(char **));
   if (!argv) gpm_report(GPM_PR_OOPS,GPM_MESS_ALLOC_FAILED);
   argv[0] = argv0;

   if (!str) {
      *argcptr = argc; /* 1 */
      return argv;
   }
   /* else, add arguments */
   s = argv[1] = strdup(str);
   argc = 2; /* first to fill */

   /* ok, now split: the first one is in place, and s is the whole string */
   for ( ; sep && (s = strchr(s, sep)) ; argc++) {
      *s = '\0';
      s++;
      argv[argc] = s;
   }
   *argcptr = argc;
   return argv;
}
