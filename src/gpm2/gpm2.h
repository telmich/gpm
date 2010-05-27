/*
 * Copyright (c) 2010    Nico Schottelius <nico-gpm2 () schottelius.org>
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
 *   Exported ressources for the library
 */

#ifndef _GPM2_H
#define _GPM2_H

struct gpm2_mouse_info {
   int fd;                    /* opened device        */
   char *name;                /* name = file          */
};

struct gpm2_mouse_proto {
   char *name;                /* identifier                 */
   char *desc;                /* description                */

   int   (*init)(struct gpm2_mouse_info *);     /* init mouse    */
   int   (*handle)(struct gpm2_mouse_info *);   /* handle mouse  */
   int   (*close)(struct gpm2_mouse_info *);    /* close mouse   */

};

struct gpm2_mouse {
   struct gpm2_mouse_info info;     /* mouse information    */
   struct gpm2_mouse_proto *proto;  /* protocol used        */
   struct gpm2_mouse *next;
};

struct gpm2_event {
   struct gpm2_mouse mouse;   /* which mouse reported the event? */
};


#endif
