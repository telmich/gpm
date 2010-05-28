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
 *   Init protocols
 *
 */

#include <fcntl.h>              /* open */
#include <unistd.h>             /* close/dup2 */
#include <stdio.h>              /* perror */
#include <stdlib.h>             /* malloc */

#include <gpm2.h>

struct gpm2_mouse_proto protocols[2];

int proto_init()
{

   protocols = {
      { "imps2", "Microsoft Intellimouse (ps2)", i_imps2, h_imps2, c_imps2 },
      { "", "", NULL, NULL, NULL }
   };

   return 1;
}
