
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

#include <ctype.h>

/* Convert an escape sequence to a single byte */
int twiddler_escape_sequence(char *s, int *len)
{
   int res = 0;

   *len = 0;
   if(isdigit(*s)) {            /* octal */
      while(isdigit(*s)) {
         res *= 8;
         res += *s - '0';
         s++;
         (*len)++;
         if(*len == 3)
            return res;
      }
      return res;
   }

   *len = 1;
   switch (*s) {
      case 'n':
         return '\n';
      case 'r':
         return '\r';
      case 'e':
         return '\e';
      case 't':
         return '\t';
      case 'a':
         return '\a';
      case 'b':
         return '\b';
      default:
         return *s;

         /*
          * a case statement after default: ???? 
          */
      case 'x':                /* hex */
         (*len)++;
         s++;
         if(isdigit(*s))
            res = *s - '0';
         else
            res = tolower(*s) - 'a' + 10;
         s++;
         if(isxdigit(*s)) {
            (*len)++;
            res *= 16;
            if(isdigit(*s))
               res += *s - '0';
            else
               res += tolower(*s) - 'a' + 10;
         }
         return res;
   }
}
