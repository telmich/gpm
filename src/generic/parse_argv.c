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

#include <string.h>                 /* str*              */
#include <ctype.h>                  /* isalnum           */
#include <stdio.h>                  /* fprintf           */

#include "headers/message.h"        /* messaging in gpm */
#include "headers/daemon.h"         /* daemon internals */

int parse_argv(argv_helper *info, int argc, char **argv)
{
   int i, j = 0, errors = 0;
   long l;
   argv_helper *p;
   char *s, *t;
   int base = 0; /* for strtol */


   for (i=1; i<argc; i++) {
      for (p = info; p->type != ARGV_END; p++) {
         j = strlen(p->name);
         if (strncmp(p->name, argv[i], j))
            continue;
         if (isalnum(argv[i][j]))
            continue;
         break;
      }
      if (p->type == ARGV_END) { /* not found */
         fprintf(stderr, "%s: Uknown option \"%s\" for pointer \"%s\"\n",
                  option.progname, argv[i], argv[0]);
         errors++;
         continue;
      }
      /* Found. Look for trailing stuff, if any */
      s = argv[i]+j;
      while (*s && isspace(*s)) s++; /* skip spaces */
      if (*s == '=') s++; /* skip equal */
      while (*s && isspace(*s)) s++; /* skip other spaces */

      /* Now parse what s is */
      switch(p->type) {
         case ARGV_BOOL:
            if (*s) {
               gpm_report(GPM_PR_ERR,GPM_MESS_OPTION_NO_ARG, option.progname,p->name,s);
               errors++;
            }
            *(p->u.iptr) = p->value;
            break;

         case ARGV_DEC:
            base = 10; /* and fall through */
         case ARGV_INT:
            l = strtol(s, &t, base);
            if (*t) {
               gpm_report(GPM_PR_ERR,GPM_MESS_INVALID_ARG, option.progname, s, p->name);
               errors++;
               break;
            }
            *(p->u.iptr) = (int)l;
            break;

         case ARGV_STRING:
            *(p->u.sptr) = strdup(s);
            break;

         case ARGV_END: /* let's please "-Wall" */
            break;
      }
   } /* for i in argc */
   if (errors) gpm_report(GPM_PR_ERR,GPM_MESS_CONT_WITH_ERR, option.progname);
   return errors;
}
