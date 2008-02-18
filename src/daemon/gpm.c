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

/* who the f*** runs gpm without glibc? doesn't have dietlibc __socklent_t? */
#if !defined(__GLIBC__)
   typedef unsigned int __socklen_t;
#endif    /* __GLIBC__ */

#ifndef max
#define max(a,b) ((a)>(b) ? (a) : (b))
#endif

/* global variables */
struct options option;        /* one should be enough for us */

/* FIXME: BRAINDEAD..ok not really, but got to leave anyway... */
/* argc and argv for mice initialization */
int mouse_argc[3]; /* 0 for default (unused) and two mice */
char **mouse_argv[3]; /* 0 for default (unused) and two mice */


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
int opt_aged = 0;
char *opt_special=NULL; /* special commands, like reboot or such */
int opt_rawrep=0;
Gpm_Type *repeated_type=0;

struct winsize win;
int maxx, maxy;
int fifofd=-1;

int eventFlag=0;
Gpm_Cinfo *cinfo[MAX_VC+1];
fd_set selSet, readySet, connSet;

time_t last_selection_time;
time_t opt_age_limit = 0;


/***********************************************************************
 * Clean global variables
 */
int opt_resize=0; /* not really an option */


/***********************************************************************
 * UNClean global section
 */
static int statusX,statusY,statusB; /* to return info */
static int statusC=0; /* clicks */
void get_console_size(Gpm_Event *ePtr);

/*===================================================================*/
/*
 *      first, all the stuff that used to be in gpn.c (i.e., not main-loop)
 */
/*-------------------------------------------------------------------*/

static inline void selection_copy(int x1, int y1, int x2, int y2, int mode)
{
/*
 * The approach in "selection" causes a bus error when run under SunOS 4.1
 * due to alignment problems...
 */
   unsigned char buf[6*sizeof(short)];
   unsigned short *arg = (unsigned short *)buf + 1;
   int fd;

   buf[sizeof(short)-1] = 2;  /* set selection */

   arg[0]=(unsigned short)x1;
   arg[1]=(unsigned short)y1;
   arg[2]=(unsigned short)x2;
   arg[3]=(unsigned short)y2;
   arg[4]=(unsigned short)mode;

   if ((fd=open_console(O_WRONLY))<0)
      gpm_report(GPM_PR_OOPS,GPM_MESS_OPEN_CON);
   /* FIXME: should be replaced with string constant (headers/message.h) */
   gpm_report(GPM_PR_DEBUG,"ctl %i, mode %i",(int)*buf, arg[4]);
   if (ioctl(fd, TIOCLINUX, buf+sizeof(short)-1) < 0)
     gpm_report(GPM_PR_OOPS,GPM_MESS_IOCTL_TIOCLINUX);
   close(fd);

   if (mode < 3) {
      opt_aged = 0;
      last_selection_time = time(0);
   }
}


/*-------------------------------------------------------------------*/
/* comment missing; FIXME */
/*-------------------------------------------------------------------*/
static inline void selection_paste(void)
{
   char c=3;
   int fd;

   if (!opt_aged && (0 != opt_age_limit) &&
      (last_selection_time + opt_age_limit < time(0))) {
      opt_aged = 1;
   }

   if (opt_aged) {
      gpm_report(GPM_PR_DEBUG,GPM_MESS_SKIP_PASTE);
      return;
   } 

   fd=open_console(O_WRONLY);
   if(ioctl(fd, TIOCLINUX, &c) < 0)
      gpm_report(GPM_PR_OOPS,GPM_MESS_IOCTL_TIOCLINUX);
   close(fd);
}

/*-------------------------------------------------------------------*/
static  inline int do_selection(Gpm_Event *event)  /* returns 0, always */
{
   static int x1=1, y1=1, x2, y2;
#define UNPOINTER() 0

   x2=event->x; y2=event->y;
   switch(GPM_BARE_EVENTS(event->type)) {
      case GPM_MOVE:
         if (x2<1) x2++; else if (x2>maxx) x2--;
         if (y2<1) y2++; else if (y2>maxy) y2--;
         selection_copy(x2,y2,x2,y2,3); /* just highlight pointer */
         return 0;

      case GPM_DRAG:
         if (event->buttons==GPM_B_LEFT) {
            if (event->margin) /* fix margins */
               switch(event->margin) {
                  case GPM_TOP: x2=1; y2++; break;
                  case GPM_BOT: x2=maxx; y2--; break;
                  case GPM_RGT: x2--; break;
                  case GPM_LFT: y2<=y1 ? x2++ : (x2=maxx, y2--); break;
               }
            selection_copy(x1,y1,x2,y2,event->clicks);
            if (event->clicks>=opt_ptrdrag && !event->margin) /* pointer */
               selection_copy(x2,y2,x2,y2,3);
         } /* if */
         return 0;

      case GPM_DOWN:
         switch (event->buttons) {
            case GPM_B_LEFT:
               x1=x2; y1=y2;
               selection_copy(x1,y1,x2,y2,event->clicks); /* start selection */
               return 0;

            case GPM_B_MIDDLE:
               selection_paste();
               return 0;

            case GPM_B_RIGHT:
               if (opt_three==1)
                  selection_copy(x1,y1,x2,y2,event->clicks);
               else
                  selection_paste();
               return 0;
         }
   } /* switch above */
   return 0;
}

/*-------------------------------------------------------------------*/
/* returns 0 if the event has not been processed, and 1 if it has */
static inline int do_client(Gpm_Cinfo *cinfo, Gpm_Event *event)
{
   Gpm_Connect info=cinfo->data;
   int fd=cinfo->fd;
   /* value to return if event is not used */
   int res = !(info.defaultMask & event->type);

   /* instead of returning 0, scan the stack of clients */
   if ((info.minMod & event->modifiers) < info.minMod)
      goto scan;
   if ((info.maxMod & event->modifiers) < event->modifiers) 
      goto scan;

   /* if not managed, use default mask */
   if (!(info.eventMask & GPM_BARE_EVENTS(event->type))) {
      if (res) return res;
      else goto scan;
   }   

   /* WARNING */ /* This can generate a SIGPIPE... I'd better catch it */
   MAGIC_P((write(fd,&magic, sizeof(int))));
   write(fd,event, sizeof(Gpm_Event));

   return info.defaultMask & GPM_HARD ? res : 1; /* HARD forces pass-on */

   scan:
   if (cinfo->next != 0) 
      return do_client (cinfo->next, event); /* try the next */
   return 0; /* no next, not used */
}

/*-------------------------------------------------------------------
 * fetch the actual device data from the mouse device, dependent on
 * what Gpm_Type is being passed.
 *-------------------------------------------------------------------*/
static inline char *getMouseData(int fd, Gpm_Type *type, int kd_mode)
{
   static unsigned char data[32]; /* quite a big margin :) */
   char *edata=data+type->packetlen;
   int howmany=type->howmany;
   int i,j;

/*....................................... read and identify one byte */

   if (read(fd, data, howmany)!=howmany) {
      if (opt_test) exit(0);
      gpm_report(GPM_PR_ERR,GPM_MESS_READ_FIRST, strerror(errno));
      return NULL;
   }

   if (kd_mode!=KD_TEXT && fifofd != -1 && opt_rawrep)
      write(fifofd, data, howmany);

   if ((data[0]&(m_type->proto)[0]) != (m_type->proto)[1]) {
      if (m_type->getextra == 1) {
         data[1]=GPM_EXTRA_MAGIC_1; data[2]=GPM_EXTRA_MAGIC_2;
         gpm_report(GPM_PR_DEBUG,GPM_EXTRA_DATA,data[0]);
         return data;
      }
      gpm_report(GPM_PR_DEBUG,GPM_MESS_PROT_ERR);
      return NULL;
   }

/*....................................... read the rest */

   /*
    * well, this seems to work almost right with ps2 mice. However, I've never
    * tried ps2 with the original selection package, which called usleep()
    */
     
   if((i=m_type->packetlen-howmany)) /* still to get */
      do {
         j = read(fd,edata-i,i); /* edata is pointer just after data */
         if (kd_mode!=KD_TEXT && fifofd != -1 && opt_rawrep && j > 0)
            write(fifofd, edata-i, j);
         i -= j;
      } while (i && j);

   if (i) {
      gpm_report(GPM_PR_ERR,GPM_MESS_READ_REST, strerror(errno));
      return NULL;
   }
     
   if ((data[1]&(m_type->proto)[2]) != (m_type->proto)[3]) {
      gpm_report(GPM_PR_INFO,GPM_MESS_SKIP_DATA);
      return NULL;
   }
   gpm_report(GPM_PR_DEBUG,GPM_MESS_DATA_4,data[0],data[1],data[2],data[3]);
   return data;
}

