/*
 * optparser.c - GPM mouse options parser
 *
 * Copyright (C) 1993        Andrew Haylett <ajh@gec-mrc.co.uk>
 * Copyright (C) 1994-2000   Alessandro Rubini <rubini@linux.it>
 * Copyright (C) 1998,1999   Ian Zimmerman <itz@rahul.net>
 * Copyright (C) 2001,2002   Nico Schottelius <nico-gpm@schottelius.org>
 * Copyright (C) 2003        Dmitry Torokhov <dtor@mail.ru>
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
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "headers/gpmInt.h"
#include "headers/message.h"
#include "headers/optparser.h"

int parse_options(const char *proto, const char *opts, char sep, struct option_helper *info)
{
   int len, n, n_opts = 0, errors = 0;
   long l;
   struct option_helper *p;
   char *s, *t, *str;
   int base; /* for strtol */

   for (p = info; p->type != OPT_END; p++)
      p->present = 0;

   if (!opts)
      return 0;

   if (!(str = strdup(opts)))
      gpm_report(GPM_PR_OOPS, GPM_MESS_ALLOC_FAILED);

   /* split input string */
   for (s = str, n = 1; sep && (s = strchr(s, sep)); s++, n++)
        *s = '\0';

   for (s = str; n; s += strlen(s) + 1, n--) {
      if (strlen(s) == 0)
         continue;

      for (p = info; p->type != OPT_END; p++) {
         len = strlen(p->name);
         if (!strncmp(p->name, s, len) && !isalnum(s[len]))
            break;
      }
      if (p->type == OPT_END) { /* not found */
         gpm_report(GPM_PR_ERR, "%s: Uknown option \"%s\" for protocol \"%s\"\n",
                    option.progname, s, proto);
         errors++;
         continue;
      }
      if (p->present) {
         gpm_report(GPM_PR_ERR, "%s: option \"%s\" has already been seen, ignored (\"%s\")\n",
                    option.progname, s, proto);
         continue;
      }
      p->present = 1;
      n_opts++;
      /* Found. Look for trailing stuff, if any */
      s += strlen(p->name);
      while (*s && isspace(*s)) s++; /* skip spaces */
      if (*s == '=') s++; /* skip equal */
      while (*s && isspace(*s)) s++; /* skip other spaces */

      /* Now parse what s is */
      base = 0;
      switch(p->type) {
         case OPT_BOOL:
            if (*s) {
               gpm_report(GPM_PR_ERR, GPM_MESS_OPTION_NO_ARG, option.progname, p->name, s);
               errors++;
            }
            *(p->u.iptr) = p->value;
            break;

         case OPT_DEC:
            base = 10; /* and fall through */

         case OPT_INT:
            if (*s == '\0') {
               gpm_report(GPM_PR_ERR, GPM_MESS_MISSING_ARG, option.progname, p->name);
            } else {
               l = strtol(s, &t, base);
               if (*t) {
                  gpm_report(GPM_PR_ERR, GPM_MESS_INVALID_ARG, option.progname, s, p->name);
                  errors++;
                  break;
               }
               *(p->u.iptr) = (int)l;
            }
            break;

         case OPT_STRING:
            if (*s == '\0')
               gpm_report(GPM_PR_ERR, GPM_MESS_MISSING_ARG, option.progname, p->name);
            else
               *(p->u.sptr) = strdup(s);
            break;

         case OPT_END: /* let's please "-Wall" */
            break;
      }
   }

   free(str);

   if (errors) {
      gpm_report(GPM_PR_ERR,GPM_MESS_CONT_WITH_ERR, option.progname);
      return -errors;
   }
   return n_opts;
}

int check_no_options(const char *proto, const char *opts, char sep)
{
   static struct option_helper info[] = {
      { "", OPT_END }
   };

   return parse_options(proto, opts, sep, info) == 0;
}

int is_option_present(struct option_helper *info, const char *name)
{
   struct option_helper *p;
   int len;

   for (p = info; p->type != OPT_END; p++) {
      len = strlen(p->name);
      if (!strncmp(p->name, name, len) && !isalnum(name[len]))
         return p->present;
   }

   gpm_report(GPM_PR_ERR, "%s: Uknown option \"%s\"\n", option.progname, name);
   return 0;
}

