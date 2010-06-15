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
#include <ctype.h>                  /* isspace           */

#include "headers/message.h"        /* messaging in gpm */
#include "headers/daemon.h"         /* daemon internals */

/*****************************************************************************
 * the function returns a valid type pointer or NULL if not found
  *****************************************************************************/
struct Gpm_Type *find_mouse_by_name(char *name)
{
   Gpm_Type *type;
   char *s;
   int len = strlen(name);

   for (type=mice; type->fun; type++) {
      if (!strcasecmp(name, type->name)) break;
      /* otherwise, look in the synonym list */
      for (s = type->synonyms; s; s = strchr(s, ' ')) {
         while (*s && isspace(*s)) s++; /* skip spaces */
         if(!strncasecmp(name, s, len) && !isprint(*(s + len))) break;/*found*/
      }
      if(s) break; /* found a synonym */
   }
   if (!type->fun) return NULL;
   return type;
}

