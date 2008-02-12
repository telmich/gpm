/*
 * gpn.c - support functions for gpm-Linux
 *
 * Copyright (C) 1993        Andreq Haylett <ajh@gec-mrc.co.uk>
 * Copyright (C) 1994-1999   Alessandro Rubini <rubini@linux.it>
 * Copyright (C) 1998 	     Ian Zimmerman <itz@rahul.net>
 * Copyright (c) 2001 	     Nico Schottelius <nico@schottelius.org>
 *
 * Tue,  5 Jan 1999 23:26:10 +0000, modified by James Troup <james@nocrew.org>
 * (usage): typo (s/an unexistent/a non-existent/)
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
#include <string.h>        /* strerror(); ?!? memcpy() */
#include <ctype.h>         /* isdigit */
#include <signal.h>
#include <stdarg.h>        /* Log uses it */
#include <errno.h>
#include <unistd.h>        /* getopt(),symlink() */
#include <sys/stat.h>      /* mkdir()  */
#include <sys/param.h>
#include <sys/time.h>      /* timeval */
#include <sys/wait.h>      /* wait() */
#include <sys/types.h>     /* socket() */
#include <sys/socket.h>    /* socket() */
#include <sys/un.h>        /* struct sockaddr_un */
#include <asm/types.h>     /* __u32  */

#ifdef	SIGTSTP		/* true if BSD system */
#include <sys/file.h>
#include <sys/ioctl.h>
#endif

#ifndef HAVE___U32
# ifndef _I386_TYPES_H /* /usr/include/asm/types.h */
typedef unsigned int __u32;
# endif
#endif

#include "headers/message.h"
#include "headers/gpmInt.h"
#include "headers/gpm.h"

extern int errno;

/*===================================================================*/
/* octal digit */
static int isodigit(const unsigned char c)
{
   return ((c & ~7) == '0');
}

/* routine to convert digits from octal notation (Andries Brouwer) */
static int getsym(const unsigned char *p0, unsigned char *res)
{
   const unsigned char *p = p0;
   char c;

   c = *p++;
   if (c == '\\' && *p) {
      c = *p++;
      if (isodigit(c)) {
         c -= '0';
         if (isodigit(*p)) c = 8*c + (*p++ - '0');
         if (isodigit(*p)) c = 8*c + (*p++ - '0');
      }
   }
   *res = c;
   return (p - p0);
}

/* description missing! FIXME */
int loadlut(char *charset)
{
   int i, c, fd;
   unsigned char this, next;
   static __u32 long_array[9]={
      0x05050505, /* ugly, but preserves alignment */
      0x00000000, /* control chars     */
      0x00000000, /* digits            */
      0x00000000, /* uppercase and '_' */
      0x00000000, /* lowercase         */
      0x00000000, /* Latin-1 control   */
      0x00000000, /* Latin-1 misc      */
      0x00000000, /* Latin-1 uppercase */
      0x00000000  /* Latin-1 lowercase */
   };


#define inwordLut (long_array+1)

   for (i=0; charset[i]; ) {
      i += getsym(charset+i, &this);
      if (charset[i] == '-' && charset[i + 1] != '\0')
         i += getsym(charset+i+1, &next) + 1;
      else
         next = this;
      for (c = this; c <= next; c++)
         inwordLut[c>>5] |= 1 << (c&0x1F);
   }
  
   if ((fd=open(option.consolename, O_WRONLY)) < 0) {
      /* try /dev/console, if /dev/tty0 failed -- is that really senseful ??? */
      free(option.consolename); /* allocated by main */
      if((option.consolename=malloc(strlen(GPM_SYS_CONSOLE)+1)) == NULL)
         gpm_report(GPM_PR_OOPS,GPM_MESS_NO_MEM);
      strcpy(option.consolename,GPM_SYS_CONSOLE);
     
      if ((fd=open(option.consolename, O_WRONLY)) < 0) gpm_report(GPM_PR_OOPS,GPM_MESS_OPEN_CON);
   }
   if (ioctl(fd, TIOCLINUX, &long_array) < 0) { /* fd <0 is checked */
      if (errno==EPERM && getuid())
         gpm_report(GPM_PR_WARN,GPM_MESS_ROOT); /* why do we still continue?*/
      else if (errno==EINVAL)
         gpm_report(GPM_PR_OOPS,GPM_MESS_CSELECT);
   }
   close(fd);

   return 0;
}

/* usage: display for usage informations */
int usage(char *whofailed)
{
   if (whofailed) {
      gpm_report(GPM_PR_ERR,GPM_MESS_SPEC_ERR,whofailed,option.progname);
      return 1;
   }
   printf(GPM_MESS_USAGE,option.progname, DEF_ACCEL, DEF_BAUD, DEF_SEQUENCE,
   DEF_DELTA, DEF_TIME, DEF_LUT,DEF_SCALE, DEF_SAMPLE, DEF_TYPE);
   return 1;
}

/* itz Sat Sep 12 10:55:51 PDT 1998 Added this as replacement for the
   unwanted functionality in check_uniqueness. */

void check_kill(void)
{
   int old_pid;
   FILE* fp = fopen(GPM_NODE_PID, "r");

   /* if we cannot find the old pid file, leave */
   if (fp == NULL) gpm_report(GPM_PR_OOPS,GPM_MESS_OPEN, GPM_NODE_PID);
  
   /* else read the pid */
   if (fscanf(fp,"%d",&old_pid) != 1)
      gpm_report(GPM_PR_OOPS,GPM_MESS_READ_PROB,GPM_NODE_PID);
   fclose(fp);

   gpm_report(GPM_PR_DEBUG,GPM_MESS_KILLING,old_pid);

   /* first check if we run */
   if (kill(old_pid,0) == -1) {
      gpm_report(GPM_PR_INFO,GPM_MESS_STALE_PID, GPM_NODE_PID);
      unlink(GPM_NODE_PID);
   }
   /* then kill us (not directly, but the other instance ... ) */
   if (kill(old_pid,SIGTERM) == -1)
      gpm_report(GPM_PR_OOPS,GPM_MESS_CANT_KILL, old_pid);

   gpm_report(GPM_PR_INFO,GPM_MESS_KILLED,old_pid);
   exit(0);
}

/* itz Sat Sep 12 10:30:05 PDT 1998 this function used to mix two
   completely different things; opening a socket to a running daemon
   and checking that a running daemon existed.  Ugly. */
/* rewritten mostly on 20th of February 2002 - nico */   
void check_uniqueness(void)
{
   FILE *fp    =  0;
   int old_pid = -1;

   if((fp = fopen(GPM_NODE_PID, "r")) != NULL) {
      fscanf(fp, "%d", &old_pid);
      if (kill(old_pid,0) == -1) {
         gpm_report(GPM_PR_INFO,GPM_MESS_STALE_PID, GPM_NODE_PID);
         unlink(GPM_NODE_PID);
      } else /* we are really running, exit asap! */ 
         gpm_report(GPM_PR_OOPS,GPM_MESS_ALREADY_RUN, old_pid);
   }
   /* now try to sign ourself */
   if ((fp = fopen(GPM_NODE_PID,"w")) != NULL) {
      fprintf(fp,"%d\n",getpid());
      fclose(fp);
   } else {
      gpm_report(GPM_PR_OOPS,GPM_MESS_NOTWRITE,GPM_NODE_PID);
   }
}

/*****************************************************************************
 * the function returns a valid type pointer or NULL if not found
 *****************************************************************************/
struct Gpm_Type *find_mouse_by_name(char *name)
{
   Gpm_Type *type;
   char *s;
   int len = strlen(name);

   for (type=mice; type->fun; type++) {
      if (!strcasecmp(name, type->name)) break;
      /* otherwise, look in the synonym list */
      for (s = type->synonyms; s; s = strchr(s, ' ')) {
         while (*s && isspace(*s)) s++; /* skip spaces */
         if(!strncasecmp(name, s, len) && !isprint(*(s + len))) break;/*found*/
      }
   	if(s) break; /* found a synonym */
   }
   if (!type->fun) return NULL;
   return type;
}

/*****************************************************************************
 * Check the command line and / or set the appropriate variables
 * Can't believe it, today cmdline() really does what the name tries to say
 *****************************************************************************/
void cmdline(int argc, char **argv)
{
   extern struct options option;
   char options[]="a:A::b:B:d:Dg:hi:kl:m:Mo:pr:R::s:S:t:TuvV::23";
   int  opt;

   /* initialize for the dual mouse */ 
   mouse_table[2]=mouse_table[1]=mouse_table[0]; /* copy defaults */
   which_mouse=mouse_table+1; /* use the first */

   while ((opt = getopt(argc, argv, options)) != -1) {
      switch (opt) {
         case 'a': opt_accel = atoi(optarg);             break;
         case 'A': opt_aged++;
                   if (optarg)
                     opt_age_limit = atoi(optarg);       break;
         case 'b': opt_baud = atoi(optarg);              break;
         case 'B': opt_sequence = optarg;                break;
         case 'd': opt_delta = atoi(optarg);             break;
         case 'D': option.run_status = GPM_RUN_DEBUG;    break;
         case 'g': opt_glidepoint_tap=atoi(optarg);      break;
         case 'h': exit(usage(NULL));
         case 'i': opt_time=atoi(optarg);                break;
         case 'k': check_kill();                         break;
         case 'l': opt_lut = optarg;                     break;
         case 'm': add_mouse(GPM_ADD_DEVICE,optarg);     
                   opt_dev = optarg;                     break; /* GO AWAY!*/
         case 'M': opt_double++; option.repeater++;
            if (option.repeater_type == 0)
               option.repeater_type = "msc";
            which_mouse=mouse_table+2;                   break;
         case 'o': add_mouse(GPM_ADD_OPTIONS,optarg);
                   gpm_report(GPM_PR_DEBUG,"options: %s",optarg);
                   opt_options = optarg;                 break; /* GO AWAY */
         case 'p': opt_ptrdrag = 0;                      break;
         case 'r':
            /* being called responsiveness, I must take the inverse */
            opt_scale=atoi(optarg);
            if(!opt_scale || opt_scale > 100) opt_scale=100; /* the maximum */
            else opt_scale=100/opt_scale;                break;
         case 'R':
            option.repeater++;
            if (optarg) option.repeater_type = optarg;
            else        option.repeater_type = "msc";    break;
         case 's': opt_sample = atoi(optarg);            break;
         case 'S': if (optarg) opt_special = optarg;
                   else opt_special="";                  break;
         case 't': add_mouse(GPM_ADD_TYPE,optarg);
                   opt_type = optarg;                    break; /* GO AWAY */
         case 'u': option.autodetect = 1;                break;
         case 'T': opt_test++;                           break;
         case 'v': printf(GPM_MESS_VERSION "\n");        exit(0);
         case '2': opt_three = -1;                       break;
         case '3': opt_three =  1;                       break;
         default: exit(usage("commandline"));
      }
   }
}
