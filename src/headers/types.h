/*
 * Types used by gpm internally only
 *
 * Copyright (c) 2008    Nico Schottelius <nico-gpm2008 at schottelius.org>
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
 */

#ifndef _GPM_TYPE_H
#define _GPM_TYPE_H

/*************************************************************************
 * Includes
 */


/*************************************************************************
 * Types / structures
 */

typedef struct Gpm_Cinfo {
   Gpm_Connect data;
   int fd;
   struct Gpm_Cinfo *next;
} Gpm_Cinfo;


/* and this is the entry in the mouse-type table */
typedef struct Gpm_Type {
   char              *name;
   char              *desc;      /* a descriptive line */
   char              *synonyms;  /* extra names (the XFree name etc) as a list */
   /* mouse specific event handler: */
   int               (*fun)(Gpm_Event *state, unsigned char *data);

   /* mouse specific initialisation function: */
   struct Gpm_Type   *(*init)(int fd, unsigned short flags,
                     struct Gpm_Type   *type, int argc, char **argv);

   unsigned short    flags;
   unsigned char     proto[4];
   int               packetlen;
   int               howmany;    /* how many bytes to read at a time */
   int               getextra;   /* does it get an extra byte? (only mouseman) */
   int               absolute;   /* flag indicating absolute pointing device */

                     /* repeat this event into fd */
   int               (*repeat_fun)(Gpm_Event *state, int fd);
} Gpm_Type;


#endif
