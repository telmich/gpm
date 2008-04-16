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
 *    data definitions used by clients and daemon
 ********/

/* structs */
struct gpm2_mouse {
   char *name;
   char *dev;
   int  fd;
};

struct gpm2_me {        /* mouse element */
   struct gpm2_mouse mouse;
   struct gpm2_me *next;
   struct gpm2_me *prev;
};

struct gpm2_packet_raw() {    /* raw mouse packets */
   char *data;
   int   len;
};

/* must contain everything a mouse can do */
struct gpm2_packet() {    /* decoded mouse packets */
   
   int nr_buttons;
   int *buttons;

   int pos_type;           /* absolute or relativ */
};

/* structure */
struct gpm2_mic

/* gpm2-daemon needs a list of mice, containing:
 * - name of mouse
 * - device used
 * - filedescriptor
 * - mouse configuration as found in cconfig
 */
