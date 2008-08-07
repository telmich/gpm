
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

/* These are the special functions that perform special actions */
int twiddler_console(char *s)
{
   int consolenr = atoi(s);     /* atoi never fails :) */

   int fd;

   if(consolenr == 0)
      return 0;                 /* nothing to do */

   fd = open_console(O_RDONLY);
   if(fd < 0)
      return -1;
   if(ioctl(fd, VT_ACTIVATE, consolenr) < 0) {
      close(fd);
      return -1;
   }

/*if (ioctl(fd, VT_WAITACTIVE, consolenr)<0) {close(fd); return -1;} */
   close(fd);
   return 0;
}
