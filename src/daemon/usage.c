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

#include "headers/message.h"        /* messaging in gpm */
#include "headers/daemon.h"         /* daemon internals */

int usage(char *whofailed)
{
   if (whofailed) {
      gpm_report(GPM_PR_ERR,GPM_MESS_SPEC_ERR,whofailed,option.progname);
      return 1;
   }
   printf(GPM_MESS_USAGE,option.progname, DEF_ACCEL, DEF_BAUD, DEF_SEQUENCE,
   DEF_DELTA, DEF_TIME, DEF_LUT,DEF_SCALE, DEF_SAMPLE, DEF_TYPE);
   return 1;
}

