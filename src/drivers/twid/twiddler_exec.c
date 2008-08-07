
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

int twiddler_exec(char *s)
{
   int pid;

   extern struct options option;

   switch (pid = fork()) {
      case -1:
         return -1;
      case 0:
         close(0);
         close(1);
         close(2);              /* very rude! */

         open(GPM_NULL_DEV, O_RDONLY);
         open(option.consolename, O_WRONLY);
         dup(1);
         execl("/bin/sh", "sh", "-c", s, NULL);
         exit(1);               /* shouldn't happen */

      default:                 /* father: */
         return (0);
   }
}
