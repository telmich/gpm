
/*
 * Support for the twiddler keyboard
 *
 * Copyright 1998          Alessandro Rubini (rubini at linux.it)
 * Copyright 2001 - 2008   Nico Schottelius (nico-gpm2008 at schottelius.org)
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

#include <string.h>

#include "twiddler.h"

/* retrieve the right table according to the modifier string */
const char **twiddler_mod_to_table(const char *mod)
{
   struct twiddler_map_struct *ptr;

   int len = strlen(mod);

   if(len == 0)
      return twiddler_map->table;

   for(ptr = twiddler_map; ptr->table; ptr++) {
      if(!strncasecmp(mod, ptr->keyword, len))
         return ptr->table;
   }
   return NULL;
}
