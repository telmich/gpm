/*
 * Daemon internals
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

#ifndef _GPM_DAEMON_H
#define _GPM_DAEMON_H

/*************************************************************************
 * Includes
 */
#include "gpm.h"           /* Gpm_Event         */
#include "types.h"         /* Internal types    */
#include <sys/select.h>    /* fd_set            */

/*************************************************************************
 * Types / structures
 */

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
         *opt_calib;
   int   opt_baud,
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
   char  *opt_options;           /* extra textual configuration */
   Gpm_Type *m_type;
   int fd;
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


/*************************************************************************
 * Macros
 */

/* How many virtual consoles are managed? */
#ifndef MAX_NR_CONSOLES
#  define MAX_NR_CONSOLES 64 /* this is always sure */
#endif

#define MAX_VC    MAX_NR_CONSOLES  /* doesn't work before 1.3.77 */

/* for adding a mouse; add_mouse */
#define GPM_ADD_DEVICE        0
#define GPM_ADD_TYPE          1
#define GPM_ADD_OPTIONS       2

/* all the default values */
#define DEF_TYPE          "ms"
#define DEF_DEV           NULL     /* use the type-related one */
#define DEF_LUT   "-a-zA-Z0-9_./\300-\326\330-\366\370-\377"
#define DEF_SEQUENCE     "123"     /* how buttons are reordered */
#define DEF_CALIB         NULL     /* don't load calibration data */
#define DEF_BAUD          1200
#define DEF_SAMPLE         100
#define DEF_DELTA           25
#define DEF_ACCEL            2
#define DEF_SCALE           10
#define DEF_TIME           250    /* time interval (ms) for multiple clicks */
#define DEF_THREE            0    /* have three buttons? */
#define DEF_KERNEL           0    /* no kernel module, by default */

#define DEF_DMINX	     0
#define DEF_DMAXX	     1
#define DEF_DMINY	     0
#define DEF_DMAXY	     1

#define DEF_OMINX	     0
#define DEF_OMAXX	     1
#define DEF_OMINY	     0
#define DEF_OMAXY	     1

/* 10 on old computers (<=386), 0 on current machines */
#define DEF_CLUSTER          0    /* maximum number of clustered events */

#define DEF_TEST             0
#define DEF_PTRDRAG          1    /* double or triple click */
#define DEF_GLIDEPOINT_TAP   0    /* tapping emulates no buttons by default */



/*************************************************************************
 * Global variables
 */

extern char             *opt_lut;
extern char             *opt_special;

extern int              opt_resize;       /* not really an option          */
extern time_t           opt_age_limit;
extern struct options   option;           /* one should be enough for us   */
extern int              mouse_argc[3];    /* 0 for default (unused)        */
extern char           **mouse_argv[3];    /* and two mice                  */

extern int              opt_aged,
                        opt_ptrdrag,
                        opt_test,
                        opt_double;


extern int              statusX,
                        statusY,
                        statusB,
                        statusC;          /* clicks */
extern int              fifofd;
extern int              opt_rawrep;
extern int              maxx,
                        maxy;


extern fd_set           selSet,
                        readySet,
                        connSet;
extern int              eventFlag;
extern struct winsize   win;

extern Gpm_Cinfo       *cinfo[MAX_VC+1];

extern struct mouse_features  mouse_table[3],
                             *which_mouse;      /*the current one*/

extern Gpm_Type         mice[];
extern Gpm_Type         *repeated_type;

time_t                  last_selection_time;


/*************************************************************************
 * Functions
 */

char **build_argv(char *argv0, char *str, int *argcptr, char sep);

void disable_paste(int vc);

int   do_client(Gpm_Cinfo *cinfo, Gpm_Event *event);
int   do_selection(Gpm_Event *event);

void  get_console_size(Gpm_Event *ePtr);
int   get_data(Gpm_Connect *where, int whence);
char *getMouseData(int fd, Gpm_Type *type, int kd_mode);
int   getsym(const unsigned char *p0, unsigned char *res);

void  gpm_exited(void);
void  gpm_killed(int signo);

int limit_delta(int delta, int min, int max);

int open_console(const int mode);
int old_main();
int option_modem_lines(int fd, int argc, char **argv);

int parse_argv(argv_helper *info, int argc, char **argv);

int processConn(int fd);
int processMouse(int fd, Gpm_Event *event, Gpm_Type *type, int kd_mode);
int processRequest(Gpm_Cinfo *ci, int vc);
int processSpecial(Gpm_Event *event);

void selection_copy(int x1, int y1, int x2, int y2, int mode);
void selection_paste(void);

void startup(int argc, char **argv);


int wait_text(int *fdptr);

/* meta-mouse functions */
void add_mouse (int type, char *value);
int  init_mice (struct micetab *micelist);
int  reset_mice(struct micetab *micelist);

/* gpn.c */
void cmdline(int argc, char **argv);
int giveInfo(int request, int fd);
int loadlut(char *charset);
int usage(char *whofailed);
struct Gpm_Type *find_mouse_by_name(char *name);
void check_uniqueness(void);
void check_kill(void);

/* gpm.c */
int old_main();

#endif
