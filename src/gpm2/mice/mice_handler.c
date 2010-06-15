/*
 * gpm2 - mouse driver for the console
 *
 * Copyright (c) 2007         Nico Schottelius <nico-gpm2 () schottelius.org>
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
 *    handle mice
 ********/

/* unclean headers */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>        /* close */


#include "gpm2-daemon.h"
#include "tmp/protocols.h"

int mice_handler()
{
   
   /* init_mice_handler();
    * open connections to gpm2_daemon:
    * - one for each mouse:
    *    * raw channel     (unidirectional)
    *    * decoded channel (unidirectional)
    *    * control channel (bidirectional)
    */
   init_mice_handler();

   /* init_mice:
    *
    * - open mousedev
    * - drop priviliges after that
    */
   //init_mice();


   //handle_mice();  /* forks and maintains mice */

   /* dirty hack */
   int ps2test = open("/dev/psaux",O_RDWR);

   gpm2_open_ps2(ps2test);
   gpm2_decode_ps2(ps2test);

   close(ps2test);

   return 0;
}
