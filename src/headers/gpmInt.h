/*
 * gpmInt.h - server-only stuff for gpm
 *
 * Copyright (C) 1994-1999  Alessandro Rubini <rubini@linux.it>
 * Copyright (C) 1998	    Ian Zimmerman <itz@rahul.net>
 * Copyright (C) 2001-2004  Nico Schottelius <nico-gpm@schottelius.org>
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

#include <sys/time.h>   /* timeval */
#include "gpm.h"

#if !defined(__GNUC__)
#  define inline
#endif 

/*....................................... old gpmCfg.h */
/* timeout for the select() syscall */
#define SELECT_TIME 86400 /* one day */

/* How many buttons may the mouse have? */
/* #define MAX_BUTTONS 3  ===> not used, it is hardwired :-( */

/* all the default values */
#define DEF_TYPE          "ms"
#define DEF_REP_TYPE     "msc"
#define DEF_DEV           NULL     /* use the type-related one */
#define DEF_LUT   "-a-zA-Z0-9_./\300-\326\330-\366\370-\377"
#define DEF_SEQUENCE     "123"     /* how buttons are reordered */
#define DEF_BAUD          1200
#define DEF_SAMPLE         100
#define DEF_DELTA           25
#define DEF_ACCEL            2
#define DEF_SCALE           10
#define DEF_TIME           250    /* time interval (ms) for multiple clicks */
#define DEF_THREE            0    /* have three buttons? */

/* 10 on old computers (<=386), 0 on current machines */
#define DEF_CLUSTER          0    /* maximum number of clustered events */

#define DEF_PTRDRAG          1    /* double or triple click */
#define DEF_GLIDEPOINT_TAP   0    /* tapping emulates no buttons by default */

/*....................................... Strange requests (iff conn->pid==0)*/

#define GPM_REQ_SNAPSHOT 0
#define GPM_REQ_BUTTONS  1
#define GPM_REQ_CONFIG   2
#define GPM_REQ_NOPASTE  3

/* misc :) */
#define GPM_NULL_DEV         "/dev/null"
#define GPM_SYS_CONSOLE      "/dev/console"
#define GPM_DEVFS_CONSOLE    "/dev/vc/0"
#define GPM_OLD_CONSOLE      "/dev/tty0"

/*** mouse commands ***/ 

#define GPM_AUX_SEND_ID    0xF2
#define GPM_AUX_ID_ERROR   -1
#define GPM_AUX_ID_PS2     0
#define GPM_AUX_ID_IMPS2   3

/* these are shameless stolen from /usr/src/linux/include/linux/pc_keyb.h    */

#define GPM_AUX_SET_RES        0xE8  /* Set resolution */
#define GPM_AUX_SET_SCALE11    0xE6  /* Set 1:1 scaling */ 
#define GPM_AUX_SET_SCALE21    0xE7  /* Set 2:1 scaling */
#define GPM_AUX_GET_SCALE      0xE9  /* Get scaling factor */
#define GPM_AUX_SET_STREAM     0xEA  /* Set stream mode */
#define GPM_AUX_SET_SAMPLE     0xF3  /* Set sample rate */ 
#define GPM_AUX_ENABLE_DEV     0xF4  /* Enable aux device */
#define GPM_AUX_DISABLE_DEV    0xF5  /* Disable aux device */
#define GPM_AUX_RESET          0xFF  /* Reset aux device */
#define GPM_AUX_ACK            0xFA  /* Command byte ACK. */ 

#define GPM_AUX_BUF_SIZE    2048  /* This might be better divisible by
                                 three to make overruns stay in sync
                                 but then the read function would need
                                 a lock etc - ick */
                                                      

/*....................................... Structures */

struct micedev {
   int   fd;
   int   timeout;             /* the protocol driver wants to be called 
                                 after X msec even if there is no new data
                                 arrived (-1 to disable/default) */
   void  *private;            /* private data maintained by protocol driver */
};

struct miceopt {
   char  *sequence;
   int   baud;
   int   sample;
   int   delta;
   int   accel;
   int   scalex, scaley;
   int   time;
   int   cluster;
   int   three_button;
   int   glidepoint_tap;
   int   absolute;         /* device reports absolute coordinates - initially copied
                              from Gpm_Type; allows same protocol (type) control devices
                              in absolute and relative mode */
   char  *text;            /* extra textual options supplied via '-o text' */
};

/*
 * and this is the entry in the mouse-type table
 */
typedef struct Gpm_Type {
   char *name;
   char *desc;             /* a descriptive line */
   char *synonyms;         /* extra names (the XFree name etc) as a list */
   int (*fun)(struct micedev *dev, struct miceopt *opt, unsigned char *data, Gpm_Event *state);
   int (*init)(struct micedev *dev, struct miceopt *opt, struct Gpm_Type *type);
   unsigned short flags;
   unsigned char proto[4];
   int packetlen;
   int howmany;            /* how many bytes to read at a time */
   int getextra;           /* does it get an extra byte? (only mouseman) */
   int absolute;           /* flag indicating absolute pointing device */

   int (*repeat_fun)(Gpm_Event *state, int fd); /* repeat this event into fd */
                          /* itz Mon Jan 11 23:27:54 PST 1999 */
}                   Gpm_Type;

#define GPM_EXTRA_MAGIC_1 0xAA
#define GPM_EXTRA_MAGIC_2 0x55

extern char *opt_special;
extern Gpm_Type mice[];             /* where the hell are the descriptions...*/

/* new variables <CLEAN> */

/* structure prototypes */
struct repeater {
   int      fd;
   int      raw;
   Gpm_Type *type;
};

/* contains all mice */
struct micetab {
   struct micetab *next;
   struct micedev dev;
   struct miceopt options;
   Gpm_Type       *type;
   char           *device;
   int            buttons;    /* mouse's button state from last read */
   struct timeval timestamp;  /* last time mouse data arrived */ 
};

struct options {
   int autodetect;            /* -u [aUtodetect..'A' is not available] */
   int run_status;            /* startup/daemon/debug */
   char *progname;            /* hopefully gpm ;) */
};

/* global variables */
struct options option;        /* one should be enough for us */
extern struct repeater repeater; /* again, only one */
extern struct micetab *micelist;

/* new variables </CLEAN> */

/*....................................... Prototypes */
         /* server_tools.c */
struct micetab *add_mouse(void);
void init_mice(void);
void cleanup_mice(void);

         /* startup.c */
void startup(int argc, char **argv);

         /* gpm.c */
int old_main();

       /* gpn.c */
void cmdline(int argc, char **argv);
int  giveInfo(int request, int fd);
int  usage(char *whofailed);
void check_uniqueness(void);
void kill_gpm(void);

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
