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
#include "gpm.h"     /* Gpm_Connect */

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

struct micetab {
   struct micetab *next;
   char *device;
   char *protocol;
   char *options;
};

struct options {
   int            autodetect;          /* -u [aUtodetect..'A' is unavailable] */
   int            no_mice;             /* number of mice                      */
   int            repeater;            /* repeat data                         */
   char           *repeater_type;      /* repeat data as which mouse type     */
   int            run_status;          /* startup/daemon/debug                */
   char           *progname;           /* hopefully gpm ;)                    */
   struct micetab *micelist;           /* mice and their options              */
   char           *consolename;        /* /dev/tty0 || /dev/vc/0              */
};

/* this structure is used to hide the dual-mouse stuff */
struct mouse_features {
   char  *opt_type,
         *opt_dev,
         *opt_sequence,
         *opt_calib,
         *opt_options;           /* extra textual configuration */
   int   fd,
         opt_baud,
         opt_sample,
         opt_delta,
         opt_accel,
         opt_scale,
         opt_scaley,
         opt_time,
         opt_cluster,
         opt_three,
         opt_glidepoint_tap,
         opt_dminx,
         opt_dmaxx,
         opt_dminy,
         opt_dmaxy,
         opt_ominx,
         opt_omaxx,
	      opt_ominy,
	      opt_omaxy;
   Gpm_Type *m_type;
};

/*========================================================================
 * Parsing argv: helper dats struct function
 *========================================================================*/
enum argv_type {
   ARGV_BOOL = 1,
   ARGV_INT, /* "%i" */
   ARGV_DEC, /* "%d" */
   ARGV_STRING,
   /* other types must be added */
   ARGV_END = 0
};

typedef struct argv_helper {
   char *name;
   enum argv_type type;
   union u {
      int *iptr;   /* used for int and bool arguments */
      char **sptr; /* used for string arguments, by strdup()ing the value */
   } u;
   int value; /* used for boolean arguments */
} argv_helper;


#endif
