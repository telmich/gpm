
/*
 * twiddler.c - support for the twiddler keyboard
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

/*
 * Convert the rest to a string or a byte
 * the special return value "s" signals an error
 */
char *twiddler_rest_to_value(char *s)
{
   char *ptr;

   static char buf[64];

   int ibuf = 0, len;

   struct twiddler_f_struct *sptr;

   struct twiddler_fun_struct *fptr;

   /*
    * look for the value 
    */
   ptr = strchr(s, '=');
   if(!ptr)
      return s;
   ptr++;
   while(isspace(*ptr))
      ptr++;
   if(!*ptr)
      return s;

   /*
    * now see what the rest is: accept a single char,
    * standalone escape sequences or strings (with escape seq.)
    * Whenever the string is not quoted, try to interpret it
    */
   if(*ptr == '"') {            /* string... */
      ptr++;
      while(*ptr != '"') {
         if(*ptr == '\\') {
            ptr++;
            buf[ibuf++] = twiddler_escape_sequence(ptr, &len);
            ptr += len;
         } else
            buf[ibuf++] = *(ptr++);
      }
      buf[ibuf] = '\0';
      return strdup(buf);
   }
   if(*ptr == '\\')
      return (char *) twiddler_escape_sequence(ptr + 1, &len /* unused */ );

   if(strlen(ptr) == 1)
      return ((char *) ((unsigned long) *ptr & 0xFF));

   /*
    * look for special string 
    */
   for(sptr = twiddler_f; sptr->instring; sptr++)
      if(!strcmp(ptr, sptr->instring))
         return sptr->outstring;

   /*
    * finally, look for special functions 
    */
   for(fptr = twiddler_functions; fptr->name; fptr++)
      if(!strncasecmp(fptr->name, ptr, strlen(fptr->name))) {
         if(active_fun_nr == TWIDDLER_MAX_ACTIVE_FUNS) {
            gpm_report(GPM_PR_ERR, GPM_MESS_TOO_MANY_SPECIAL, option.progname,
                       TWIDDLER_MAX_ACTIVE_FUNS);
            return s;
         }
         twiddler_active_funs[active_fun_nr].fun = fptr->fun;
         twiddler_active_funs[active_fun_nr].arg =
            strdup(ptr + strlen(fptr->name));
         return (char *) (twiddler_active_funs + active_fun_nr++);
      }

   return s;                    /* error */
}
