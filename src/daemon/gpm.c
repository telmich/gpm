/*
 * general purpose mouse support
 *
 * Copyright (C) 1993        Andreq Haylett <ajh@gec-mrc.co.uk>
 * Copyright (C) 1994-1999   Alessandro Rubini <rubini@linux.it>
 * Copyright (C) 1998        Ian Zimmerman <itz@rahul.net>
 * Copyright (c) 2001-2008   Nico Schottelius <nico-gpm2008 at schottelius.org>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>        /* strerror(); ?!?  */
#include <errno.h>
#include <unistd.h>        /* select(); */
#include <signal.h>        /* SIGPIPE */
#include <time.h>          /* time() */
#include <sys/param.h>
#include <sys/fcntl.h>     /* O_RDONLY */
#include <sys/wait.h>      /* wait()   */
#include <sys/stat.h>      /* mkdir()  */
#include <sys/time.h>      /* timeval */
#include <sys/types.h>     /* socket() */
#include <sys/socket.h>    /* socket() */
#include <sys/un.h>        /* struct sockaddr_un */

#include <linux/vt.h>      /* VT_GETSTATE */
#include <linux/serial.h>  /* for serial console check */
#include <sys/kd.h>        /* KDGETMODE */
#include <termios.h>       /* winsize */

#include "headers/gpmInt.h"   /* old daemon header */
#include "headers/message.h"

#include "headers/daemon.h"   /* clean daemon header */

/* global variables that are in daemon.h */
struct options option;        /* one should be enough for us */
Gpm_Type *repeated_type = 0;

/* FIXME: BRAINDEAD..ok not really, but got to leave anyway... */
/* argc and argv for mice initialization */
int mouse_argc[3]; /* 0 for default (unused) and two mice */
char **mouse_argv[3]; /* 0 for default (unused) and two mice */

int opt_aged = 0;
int statusX,statusY,statusB; /* to return info */

/*
 * all the values duplicated for dual-mouse operation are
 * now in this structure (see gpmInt.h)
 * mouse_table[0] is single mouse, mouse_table[1] and mouse_table[2]
 * are copied data from mouse_table[0] for dual mouse operation.
 */

struct mouse_features mouse_table[3] = {
   {
      DEF_TYPE, DEF_DEV, DEF_SEQUENCE,
      DEF_BAUD, DEF_SAMPLE, DEF_DELTA, DEF_ACCEL, DEF_SCALE, 0 /* scaley */,
      DEF_TIME, DEF_CLUSTER, DEF_THREE, DEF_GLIDEPOINT_TAP,
      (char *)NULL /* extra */,
      (Gpm_Type *)NULL,
      -1
   }
};
struct mouse_features *which_mouse;

/* These are only the 'global' options */

char *opt_lut=DEF_LUT;
int opt_test=DEF_TEST;
int opt_ptrdrag=DEF_PTRDRAG;
int opt_double=0;

char *opt_special=NULL; /* special commands, like reboot or such */
int opt_rawrep=0;

struct winsize win;
int maxx, maxy;
int fifofd=-1;

int eventFlag=0;
Gpm_Cinfo *cinfo[MAX_VC+1];

time_t last_selection_time;
time_t opt_age_limit = 0;


int opt_resize=0; /* not really an option */


int  statusC = 0; /* clicks */
void get_console_size(Gpm_Event *ePtr);

/* in daemon.h */
fd_set selSet, readySet, connSet;
