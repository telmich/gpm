
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

/* Convert the "M00L"-type string to an integer */
int twiddler_chord_to_int(char *chord)
{
   const char *convert = "0LMR";      /* 0123 */

   char *tmp;

   int result = 0;

   int shift = 0;

   if(strlen(chord) != 4)
      return -1;

   while(*chord) {
      if(!(tmp = strchr(convert, *chord)))
         return -1;
      result |= (tmp - convert) << shift;
      chord++;
      shift += 2;
   }
   return result;
}
