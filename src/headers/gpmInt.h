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

/* FIXME: still needed ?? */
/* How many virtual consoles are managed? */
#ifndef MAX_NR_CONSOLES
#  define MAX_NR_CONSOLES 64 /* this is always sure */
#endif

#define MAX_VC    MAX_NR_CONSOLES  /* doesn't work before 1.3.77 */

/* How many buttons may the mouse have? */
/* #define MAX_BUTTONS 3  ===> not used, it is hardwired :-( */

/* all the default values */
#define DEF_TYPE          "ms"
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
#define DEF_KERNEL           0    /* no kernel module, by default */

/* 10 on old computers (<=386), 0 on current machines */
#define DEF_CLUSTER          0    /* maximum number of clustered events */

#define DEF_TEST             0
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

/* for adding a mouse; add_mouse */
#define GPM_ADD_DEVICE        0
#define GPM_ADD_TYPE          1
#define GPM_ADD_OPTIONS       2

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

/*
 * and this is the entry in the mouse-type table
 */
typedef struct Gpm_Type {
  char *name;
  char *desc;             /* a descriptive line */
  char *synonyms;         /* extra names (the XFree name etc) as a list */
  int (*fun)(Gpm_Event *state, unsigned char *data);
  struct Gpm_Type *(*init)(int fd, unsigned short flags,
			   struct Gpm_Type *type, int argc, char **argv);
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

/*....................................... Global variables */

/* this structure is used to hide the dual-mouse stuff */

struct mouse_features {
  char *opt_type, *opt_dev, *opt_sequence;
  int opt_baud,opt_sample,opt_delta, opt_accel, opt_scale, opt_scaley;
  int opt_time, opt_cluster, opt_three, opt_glidepoint_tap;
  char *opt_options; /* extra textual configuration */
  Gpm_Type *m_type;
  int fd;
};

extern struct mouse_features mouse_table[3], *which_mouse; /*the current one*/

// looks unused; delete
//typedef struct Opt_struct_type {int a,B,d,i,p,r,V,A;} Opt_struct_type;

/* this is not very clean, actually, but it works fine */
#define opt_type     (which_mouse->opt_type)
#define opt_dev      (which_mouse->opt_dev)
#define opt_sequence (which_mouse->opt_sequence)
#define opt_baud     (which_mouse->opt_baud)
#define opt_sample   (which_mouse->opt_sample)
#define opt_delta    (which_mouse->opt_delta)
#define opt_accel    (which_mouse->opt_accel)
#define opt_scale    (which_mouse->opt_scale)
#define opt_scaley   (which_mouse->opt_scaley)
#define opt_time     (which_mouse->opt_time)
#define opt_cluster  (which_mouse->opt_cluster)
#define opt_three    (which_mouse->opt_three)
#define opt_glidepoint_tap (which_mouse->opt_glidepoint_tap)
#define opt_options  (which_mouse->opt_options)

#define m_type       (which_mouse->m_type)

/* the other variables */

extern char *opt_lut;
extern int opt_test, opt_ptrdrag;
extern int opt_kill;
extern int opt_kernel, opt_explicittype;
extern int opt_aged;
extern time_t opt_age_limit;
extern char *opt_special;
extern int opt_rawrep;
extern int fifofd;
extern int opt_double;

extern Gpm_Type *repeated_type;
extern Gpm_Type mice[];             /* where the hell are the descriptions...*/
extern struct winsize win;
extern int maxx, maxy;
extern Gpm_Cinfo *cinfo[MAX_VC+1];

/* new variables <CLEAN> */

/* structure prototypes */

/* contains all mice */
struct micetab {
   struct micetab *next;
   char *device;
   char *protocol;
   char *options;
};  

/* new variables </CLEAN> */


/*....................................... Prototypes */
         /* server_tools.c */
void add_mouse (int type, char *value);
int  init_mice (struct micetab *micelist);
int  reset_mice(struct micetab *micelist);

         /* startup.c */
void startup(int argc, char **argv);

         /* gpm.c */
int old_main();

       /* gpn.c */
void cmdline(int argc, char **argv);
int giveInfo(int request, int fd);
int loadlut(char *charset);
int usage(char *whofailed);
struct Gpm_Type *find_mouse_by_name(char *name);
void check_uniqueness(void);
void check_kill(void);


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
