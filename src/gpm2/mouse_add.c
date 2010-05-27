/*
 *   Copyright (c) 2010 Nico Schottelius <nico-gpm2 () schottelius.org>
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
 *
 * Add a mouse to the micelist
 *
 */

#include <fcntl.h>              /* open */
#include <unistd.h>             /* close/dup2 */
#include <stdio.h>              /* perror */
#include <stdlib.h>             /* malloc */

#include <gpm2.h>

int mouse_add(char *name, char *proto)
{
   struct gpm2_mouse *mouse = NULL;

   mouse = malloc(sizeof(struct gpm2_mouse));

   if(!mouse) return 0;

   mouse->info.name = name;
   mouse->info.fd = open(name, 0);

   if(mouse->info.fd == -1) {
      perror(name);
      return 0;
   }


}
