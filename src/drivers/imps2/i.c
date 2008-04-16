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

#include <string.h>
#include <stdio.h>   /* NULL */

#include "types.h"                  /* Gpm_type             */
#include "mice.h"                   /* GPM_AUX_ENABLE_DEV   */
#include "message.h"                /* messaging            */
#include "daemon.h"                 /* mice                 */


/* intellimouse, ps2 version: Ben Pfaff and Colin Plumb */
/* Autodetect: Steve Bennett */
Gpm_Type *I_imps2(int fd, unsigned short flags, struct Gpm_Type *type, int argc, char **argv)
{
   int id;
   static unsigned char basic_init[] = { GPM_AUX_ENABLE_DEV, GPM_AUX_SET_SAMPLE, 100 };
   static unsigned char imps2_init[] = { GPM_AUX_SET_SAMPLE, 200, GPM_AUX_SET_SAMPLE, 100, GPM_AUX_SET_SAMPLE, 80, };
   static unsigned char ps2_init[] = { GPM_AUX_SET_SCALE11, GPM_AUX_ENABLE_DEV, GPM_AUX_SET_SAMPLE, 100, GPM_AUX_SET_RES, 3, };

   flags = argc = 0; /* FIXME: 1.99.13 */
   argv = NULL;


   /* Do a basic init in case the mouse is confused */
   write_to_mouse(fd, basic_init, sizeof (basic_init));

   /* Now try again and make sure we have a PS/2 mouse */
   if (write_to_mouse(fd, basic_init, sizeof (basic_init)) != 0) {
      gpm_report(GPM_PR_ERR,GPM_MESS_IMPS2_INIT);
      return(NULL);
   }

   /* Try to switch to 3 button mode */
   if (write_to_mouse(fd, imps2_init, sizeof (imps2_init)) != 0) {
      gpm_report(GPM_PR_ERR,GPM_MESS_IMPS2_FAILED);
      return(NULL);
   }

   /* Read the mouse id */
   id = read_mouse_id(fd);
   if (id == GPM_AUX_ID_ERROR) {
      gpm_report(GPM_PR_ERR,GPM_MESS_IMPS2_MID_FAIL);
      id = GPM_AUX_ID_PS2;
   }

   /* And do the real initialisation */
   if (write_to_mouse(fd, ps2_init, sizeof (ps2_init)) != 0) {
      gpm_report(GPM_PR_ERR,GPM_MESS_IMPS2_SETUP_FAIL);
   }

   if (id == GPM_AUX_ID_IMPS2) {
   /* Really an intellipoint, so initialise 3 button mode (4 byte packets) */
      gpm_report(GPM_PR_INFO,GPM_MESS_IMPS2_AUTO);
      return type;
   }
   if (id != GPM_AUX_ID_PS2) {
      gpm_report(GPM_PR_ERR,GPM_MESS_IMPS2_BAD_ID, id);
   }
   else gpm_report(GPM_PR_INFO,GPM_MESS_IMPS2_PS2);

   for (type=mice; type->fun; type++)
      if (strcmp(type->name, "ps2") == 0) return(type);

   /* ps2 was not found!!! */
   return(NULL);
}

