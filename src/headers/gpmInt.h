/*
 * gpmInt.h - server-only stuff for gpm
 *
 * Copyright (C) 1994-1999  Alessandro Rubini <rubini@linux.it>
 * Copyright (C) 1998	    Ian Zimmerman <itz@rahul.net>
 * Copyright (C) 2001-2008  Nico Schottelius <nico-gpm2008 at schottelius.org>
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
 ********/

#ifndef _GPMINT_INCLUDED
#define _GPMINT_INCLUDED

#include <sys/types.h>          /* time_t */ /* for whom ????  FIXME */

#include "gpm.h"

#if !defined(__GNUC__)
#  define inline
#endif 

/*....................................... old gpmCfg.h */
/* timeout for the select() syscall */
#define SELECT_TIME 86400 /* one day */

#ifdef HAVE_LINUX_TTY_H
#include <linux/tty.h>
#endif

/* How many buttons may the mouse have? */
/* #define MAX_BUTTONS 3  ===> not used, it is hardwired :-( */

/* misc :) */
#define GPM_NULL_DEV         "/dev/null"
#define GPM_SYS_CONSOLE      "/dev/console"
#define GPM_DEVFS_CONSOLE    "/dev/vc/0"
#define GPM_OLD_CONSOLE      "/dev/tty0"

/*....................................... Structures */

#define GPM_EXTRA_MAGIC_1 0xAA
#define GPM_EXTRA_MAGIC_2 0x55

// looks unused; delete
//typedef struct Opt_struct_type {int a,B,d,i,p,r,V,A;} Opt_struct_type;

/* the other variables */

extern int opt_kill;
extern int opt_kernel, opt_explicittype;
extern int opt_aged;
extern time_t opt_age_limit;

/*....................................... Prototypes */

       /* mice.c */
extern int M_listTypes(void);
       /* special.c */
int processSpecial(Gpm_Event *event);
int twiddler_key(unsigned long message);
int twiddler_key_init(void);

/*....................................... Dirty hacks */

#undef GPM_USE_MAGIC /* magic token foreach message? */
#define mkfifo(path, mode) (mknod ((path), (mode) | S_IFIFO, 0)) /* startup_daemon */


#ifdef GPM_USE_MAGIC
#define MAGIC_P(code) code
#else
#define MAGIC_P(code) 
#endif

#endif /* _GPMINT_INCLUDED */
