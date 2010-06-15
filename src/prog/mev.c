/*
 * mev.c - simple client to print mouse events (gpm-Linux)
 *
 * Copyright 1994,1995   rubini@linux.it (Alessandro Rubini)
 * Copyright (C) 1998 Ian Zimmerman <itz@rahul.net>
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

/*
 * This client is meant to be used both interactively to check
 * that gpm is working, and as a background process to convert gpm events
 * to textual strings. I'm using it to handle Linux mouse
 * events to emacs
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>

#include <linux/keyboard.h> /* to use KG_SHIFT and so on */

#define ALL_KEY_MODS ((1<<KG_SHIFT)|(1<<KG_ALT)|(1<<KG_ALTGR)|(1<<KG_CTRL))

#include "headers/message.h"  /* messages */
#include "headers/gpm.h"

#define GPM_NAME "gpm"
#define GPM_DATE "X-Mas 2002"

char *prgname;
struct node {char *name; int flag;};
struct node  tableEv[]= {
   {"move",    GPM_MOVE},
   {"drag",    GPM_DRAG},
   {"down",    GPM_DOWN},
   {"up",      GPM_UP},
   {"press",   GPM_DOWN},
   {"release", GPM_UP},
   {"motion",  GPM_MOVE | GPM_DRAG},
   {"hard",    GPM_HARD},
   {"any",     ~0},
   {"all",     ~0},
   {"none",     0},
   {NULL,0}
};
struct node  tableMod[]= {
   {"shift",    1<<KG_SHIFT},
   {"anyAlt",   1<<KG_ALT | 1<<KG_ALTGR},
   {"leftAlt",  1<<KG_ALT},
   {"rightAlt", 1<<KG_ALTGR},
   {"control",  1<<KG_CTRL},
   {"any",     ~0},
   {"all",     ~0},
   {"none",     0},
   {NULL,0}
};


/* provide defaults */
int opt_mask    = ~0;           /* get everything */
int opt_default = ~GPM_HARD;    /* pass everithing unused */
int opt_minMod  =  0;           /* run always */
int opt_maxMod  = ~0;           /* with any modifier */
int opt_intrct  =  0;
int opt_vc      =  0;      /* by default get the current vc */
int opt_emacs   =  0;
int opt_fit     =  0;
int opt_pointer =  0;

/*===================================================================*/
int user_handler(Gpm_Event *event, void *data)
{
   if (opt_fit) Gpm_FitEvent(event);

   printf("mouse: event 0x%02X, at %2i,%2i (delta %2i,%2i), "
          "buttons %i, modifiers 0x%02X\r\n",
	 event->type,
	 event->x, event->y,
	 event->dx, event->dy,
	 event->buttons, event->modifiers);

   if (event->type & (GPM_DRAG|GPM_DOWN)) {
      if (0 != opt_pointer) {
         GPM_DRAWPOINTER(event);
      } /*if*/
   } /*if*/
   return 0;
}

/*-------------------------------------------------------------------*/
int emacs_handler(Gpm_Event *event, void *data)
{
   int i,j;
   static int dragX, dragY;
   static char buffer[64];

   /* itz Mon Mar 23 20:54:54 PST 1998 emacs likes the modifier bits in
   alphabetical order, so I'll use a lookup table instead of a loop; it
   is faster anyways */
   /* static char *s_mod[]={"S-","M-","C-","M-",NULL}; */
   static char *s_mod[] = {
      "",                         /* 000 */
      "S-",                       /* 001 */
      "M-",                       /* 002 */
      "M-S-",                     /* 003 */
      "C-",                       /* 004 */
      "C-S-",                     /* 005 */
      "C-M-",                     /* 006 */
      "C-M-S-",                   /* 007 */
      /* idea: maybe we should map AltGr to Emacs Alt instead of Meta? */
      "M-",                       /* 010 */
      "M-S-",                     /* 011 */
      "M-",                       /* 012 */
      "M-S-",                     /* 013 */
      "C-M-",                     /* 014 */
      "C-M-S-",                   /* 015 */
      "C-M-",                     /* 016 */
      "C-M-S-",                   /* 017 */
   };
   /* itz Mon Mar 23 08:23:14 PST 1998 what emacs calls a `drag' event
     is our `up' event with coordinates different from the `down'
     event.  What gpm calls `drag' is just `mouse-movement' to emacs. */

   static char *s_type[]={"mouse-movement", "mouse-movement","down-mouse-","mouse-",NULL};
   static char *s_button[]={"3","2","1",NULL};
   static char *s_multi[]={"double-", "triple-", 0};
   static char s_count[]="23";
   char count = '1';
   struct timeval tv_cur;
   long timestamp;
   static long dragTime;

   /* itz Mon Mar 23 08:27:53 PST 1998 this flag is needed because even
   if the final coordinates of a drag are identical to the initial
   ones, it is still a drag if there was any movement in between.  Sigh. */
   static int dragFlag = 0;
  
   gettimeofday(&tv_cur, 0);
   timestamp = ((short)tv_cur.tv_sec) * 1000 + (tv_cur.tv_usec / 1000);
   if (opt_fit) Gpm_FitEvent(event);
   buffer[0]=0;

   /* itz Sun Mar 22 19:09:04 PST 1998 Emacs doesn't understand
   modifiers on motion events. */
   if (!(event->type & (GPM_MOVE|GPM_DRAG))) {
      /* modifiers */
      strcpy(buffer, s_mod[event->modifiers & ALL_KEY_MODS]);
      /* multiple */
      for (i=0, j=GPM_DOUBLE; s_multi[i]; i++, j<<=1)
         if (event->type & j) {
            count = s_count[i];
            strcat(buffer,s_multi[i]);
         } /*if*/
   } /*if*/

   if (event->type & GPM_DRAG) {
      dragFlag = 1;
   } /*if*/

   /* itz Mon Mar 23 08:26:33 PST 1998 up-event after movement is a drag. */
   if ((event->type & GPM_UP) && dragFlag) {
      strcat(buffer, "drag-");
   } /*if*/

   /* type */
   for (i=0, j=GPM_MOVE; s_type[i]; i++, j<<=1)
      if (event->type & j)
         strcat(buffer,s_type[i]);

   /* itz Sun Mar 22 19:09:04 PST 1998 Emacs doesn't understand
   modifiers on motion events. */
   if (!(event->type & (GPM_MOVE|GPM_DRAG)))
      /* button */
      for (i=0, j=GPM_B_RIGHT; s_button[i]; i++, j<<=1)
         if (event->buttons & j)
            strcat(buffer,s_button[i]);
  
   if ((event->type & GPM_UP) && dragFlag) {
      printf("(%s ((%i . %i) %ld) %c ((%i . %i) %ld))\n",
           buffer,
           event->x, event->y, timestamp,
           count,
           dragX, dragY, dragTime);
   } else if (event->type & (GPM_DOWN|GPM_UP)) {
      printf("(%s ((%i . %i) %ld) %c)\n",
           buffer, event->x, event->y, timestamp, count);
   } else if (event->type & (GPM_MOVE|GPM_DRAG)) {
      printf("(%s ((%i . %i) %ld))\n",
           buffer, event->x, event->y, timestamp);
   } /*if*/

   if (event->type & GPM_DOWN) {
      dragX=event->x;
      dragY=event->y;
      dragTime=timestamp;
      dragFlag = 0;
   } /*if*/

   if (event->type & (GPM_DRAG|GPM_DOWN)) {
      if (0 == opt_pointer) {
         GPM_DRAWPOINTER(event);
      } /*if*/
   } /*if*/

   return 0;
}

/*===================================================================*/
int usage(void)
{
   //printf( "(" GPM_NAME ") " GPM_RELEASE ", " GPM_DATE "\n"
   printf( "(" GPM_NAME ") , " GPM_DATE "\n"
          "Usage: %s [options]\n",prgname);
   printf("  Valid options are\n"
         "    -C <number>   choose virtual console (beware of it)\n"
         "    -d <number>   choose the default mask\n"
         "    -e <number>   choose the eventMask\n"
         "    -E            emacs-mode\n"
         "    -i            accept commands from stdin\n"
         "    -f            fit drag events inside the screen\n"
         "    -m <number>   minimum modifier mask\n"
         "    -M <number>   maximum modifier mask\n"
         "    -p            show pointer while dragging\n"
         "    -u            user-mode (default)\n"
         );

   return 1;
}

/*===================================================================*/
#define PARSE_EVENTS 0
#define PARSE_MODIFIERS 1
void getmask(char *arg, int which, int* where)
{
   int last=0, value=0;
   char *cur;
   struct node *table, *n;
   int mode = 0;                 /* 0 = set, 1 = add, 2 = subtract */

   if ('+' == arg[0]) {
      mode = 1;
      ++arg;
   } else if ('-' == arg[0]) {
      mode = 2;
      ++arg;
   }

   if (isdigit(arg[0])) {
      switch(mode) {
      case 0: *where = atoi(arg); break;
      case 1: *where |= atoi(arg); break;
      case 2: *where &= ~atoi(arg); break;
      } /*switch*/
      return;
   } /*if*/

   table= (PARSE_MODIFIERS == which) ? tableMod : tableEv;
   while (1)
      {
         while (*arg && !isalnum(*arg)) arg++; /* skip delimiters */
         cur=arg;
         while(isalnum(*cur)) cur++; /* scan the word */
         if (!*cur) last++;
         *cur=0;

         for (n=table;n->name;n++)
            if (!strcmp(n->name,arg))
          {
            value |= n->flag;
            break;
          }
         if (!n->name) fprintf(stderr,"%s: Incorrect flag \"%s\"\n",prgname,arg);
         if (last) break;

         cur++; arg=cur;
      }

   switch(mode) {
   case 0: *where = value; break;
   case 1: *where |= value; break;
   case 2: *where &= ~value; break;
   } /*switch*/
}
  
/*===================================================================*/
int cmdline(int argc, char **argv, char *options)
{
   int opt;
  
   while ((opt = getopt(argc, argv, options)) != -1)
      {
         switch (opt)
            {
          /* itz Tue Mar 24 17:11:52 PST 1998 i hate options that do
             too much.  Made them orthogonal. */

            case 'C':  sscanf(optarg,"%x",&opt_vc); break;
            case 'd':  getmask(optarg, PARSE_EVENTS, &opt_default); break;
            case 'e':  getmask(optarg, PARSE_EVENTS, &opt_mask); break;
            case 'E':  opt_emacs = 1; break;
            case 'i':  opt_intrct=1; break;
            case 'f':  opt_fit=1; break;
            case 'm':  getmask(optarg, PARSE_MODIFIERS, &opt_minMod); break;
            case 'M':  getmask(optarg, PARSE_MODIFIERS, &opt_maxMod); break;
            case 'p':  opt_pointer =1; break;
            case 'u':  opt_emacs=0; break;
            default:   return 1;
            }
      }
   return 0;
}

void 
do_snapshot()
{
   Gpm_Event event;
   int i=Gpm_GetSnapshot(&event);
   char *s;

   if (-1 == i) {
      fprintf(stderr,"Warning: cannot get snapshot!\n");
      fprintf(stderr,"Have you run \"configure\" and \"make install\"?\n");
      return;
   } /*if*/

   fprintf(stderr,"Mouse has %d buttons\n",i);
   fprintf(stderr,"Currently sits at (%d,%d)\n",event.x,event.y);
   fprintf(stderr,"The window is %d columns by %d rows\n",event.dx,event.dy);
   s=Gpm_GetLibVersion(&i);
   fprintf(stderr,"The library is version \"%s\" (%i)\n",s,i);
   s=Gpm_GetServerVersion(&i);
   fprintf(stderr,"The daemon is version \"%s\" (%i)\n",s,i);
   fprintf(stderr,"The current console is %d, with modifiers 0x%02x\n",
          event.vc,event.modifiers);
   fprintf(stderr,"The button mask is 0x%02X\n",event.buttons);
}

/*===================================================================*/
int interact(char *cmd) /* returns 0 on success and !=0 on error */
{
   Gpm_Connect conn;
   int argc=0;
   char *argv[20];

   if (*cmd && cmd[strlen(cmd)-1]=='\n')
      cmd[strlen(cmd)-1]='\0';
   if (!*cmd) return 0;

   /*
   * Interaction is accomplished by building an argv and passing it to
   * cmdline(), to use the same syntax used to invoke the program
   */

   while (argc<19)
      {
         while(isspace(*cmd)) cmd++;
         argv[argc++]=cmd;
         while (*cmd && isgraph(*cmd)) cmd++;
         if (!*cmd) break;
         *cmd=0;
         cmd++;
      }
   argv[argc]=NULL;

   if (!strcmp(argv[0],"pop")) {
      return (Gpm_Close()==0 ? 1 : 0); /* a different convention on ret values */
   } /*if*/

   if (!strcmp(argv[0],"info")) {
      fprintf(stderr,"The stack of connection info is %i depth\n",gpm_flag);
      return 0;
   } /*if*/

   if (!strcmp(argv[0],"quit")) {
      exit(0);
   } /*if*/

   if (!strcmp(argv[0],"snapshot")) {
      do_snapshot();
      return 0;
   } /*if*/

   optind=0; /* scan the entire line */
   if (strcmp(argv[0],"push") || cmdline(argc,argv,"d:e:m:M:")) {
      fprintf(stderr,"Syntax error in input line\n");
      return 0;
   } /*if*/

   conn.eventMask=opt_mask;
   conn.defaultMask=opt_default;
   conn.maxMod=opt_maxMod;
   conn.minMod=opt_minMod;

   if (Gpm_Open(&conn,opt_vc)==-1)
      {
         fprintf(stderr,"%s: Can't open mouse connection\r\n",argv[0]);
         return 1;
      }
   return 0;
}

/*===================================================================*/
int main(int argc, char **argv)
{
   Gpm_Connect conn;
   char cmd[128];
   Gpm_Handler* my_handler;       /* not the real gpm handler! */
   fd_set readset;

   prgname=argv[0];

   if (cmdline(argc,argv,"C:d:e:Efim:M:pu")) exit(usage());

   gpm_zerobased = opt_emacs;
   conn.eventMask=opt_mask;
   conn.defaultMask=opt_default;
   conn.maxMod=opt_maxMod;
   conn.minMod=opt_minMod;

   if (Gpm_Open(&conn,opt_vc) == -1) {
      gpm_report(GPM_PR_ERR,"%s: Can't open mouse connection\n",prgname);
      exit(1);
   } else if (gpm_fd == -2) {
      gpm_report(GPM_PR_OOPS,"%s: use rmev to see gpm events in xterm or rxvt\n",prgname); 
   }   
   
   gpm_report(GPM_PR_DEBUG,"STILL RUNNING_1");

   my_handler= opt_emacs ? emacs_handler : user_handler;

   /* itz Sun Mar 22 09:51:33 PST 1998 needed in case the output is a pipe */
   setvbuf(stdout, 0, _IOLBF, 0);
   setvbuf(stdin, 0, _IOLBF, 0);

   while(1) {                   /* forever */
      FD_ZERO(&readset);
      FD_SET(gpm_fd, &readset);
      if (opt_intrct) {
         FD_SET(STDIN_FILENO, &readset);
      }

      if (select(gpm_fd+1, &readset, 0, 0, 0) < 0 && errno == EINTR)
         continue;
      if (FD_ISSET(STDIN_FILENO, &readset)) {
         if (0 == fgets(cmd, sizeof(cmd), stdin) || interact(cmd)) {
            exit(0);                /* ^D typed on input */
         } /*if*/
      } /*if*/
      if (FD_ISSET(gpm_fd, &readset)) {
         Gpm_Event evt;
         if (Gpm_GetEvent(&evt) > 0) {
            my_handler(&evt, 0);
         } else {
            fprintf(stderr, "mev says : Oops, Gpm_GetEvent()\n");
         }
      } /*if*/

   } /*while*/

   /*....................................... Done */

   while (Gpm_Close()); /* close all the stack */

   exit(0);
}
