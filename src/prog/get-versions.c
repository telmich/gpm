/*
 * Get gpm library and server version
 *
 * Copyright 2007-2008 Nico Schottelius <nico-gpm2008 (at) schottelius.org>
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
 *   Compile and link with: gcc -lgpm 
 *
 ********/

#include <stdio.h>            /* printf()             */
#include <gpm.h>              /* gpm information      */

int main()
{
   char *ver;
   int  intver;

   ver = Gpm_GetLibVersion(&intver);
   printf("lib: %s, %d\n",ver,intver);

   ver = Gpm_GetServerVersion(&intver);
   printf("srv: %s, %d\n",ver,intver);

   return 0;
}
