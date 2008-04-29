
/*
 * general purpose mouse (gpm)
 *
 * Copyright (c) 2008        Nico Schottelius <nico-gpm2008 at schottelius.org>
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
 ********/

#include <stdio.h>

#include "types.h"              /* Gpm_type */
#include "message.h"            /* texts */
#include "daemon.h"             /* mice */

int M_listTypes(void)
{
   Gpm_Type *type;

   printf(GPM_MESS_VERSION "\n");
   printf(GPM_MESS_AVAIL_MYT);
   for(type = mice; type->fun; type++)
      printf(GPM_MESS_SYNONYM, type->repeat_fun ? '*' : ' ',
             type->name, type->desc, type->synonyms);

   putchar('\n');

   return 1;                    /* to exit() */
}
