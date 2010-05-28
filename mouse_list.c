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
 * List mice
 *
 */

#include <gpm2.h>
#include <stdio.h>
#include "gpm2-daemon.h"

int mouse_list(struct gpm2_mouse **mouse)
{

   /* initial call */
   if(!(*mouse)) {
      *mouse = mice;
      printf("initial mouse\n");
   } else {
      printf("next mouse\n");
      *mouse = (*mouse)->next;
   }

   /* last element, which is empty */
   if(!(*mouse)) return 0;

   return 1;
}
