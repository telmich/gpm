/*
 * hltest.c - a demo program for the high-level library.
 *
 * Copyright 1995   rubini@linux.it (Alessandro Rubini)
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
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <termios.h>

#include "headers/gpm.h"

#ifndef min
#define min(a,b) ((a)>(b)?(b):(a))
#define max(a,b) ((a)>(b)?(a):(b))
#endif

/*
 * If you happen to read this source to get insights about mouse
 * management, note that most of this file is concerned with user
 * interface and screen handling. The mouse-related part begines
 * at "MOUSE-BEGIN" (so you can use your editor capabilities to
 * look for strings).
 *
 * If you use parts of this program in your own one, I'd like some credit.
 * If you want to overcome the gnu copyleft though using parts of this
 * program, just tell me about it, and I'll change the copyright.
 */



/* this is used as clientdata */

typedef struct WinInfo {
   char color;
   int number;
   short hei,wid;
   Gpm_Roi *roi;
   } WinInfo;

WinInfo left_cldata; /* for the msg bar */
  
int colors[]={0x40,0x60,0x20,0x30,0x47,0x67,0x27,0x37};
#define NR_COLORS 8
#define LEFTWID 22
/* Dirty: use escape sequences and /dev/vcs (don't want to learn curses) */

#define CLEAR (printf("\x1b[H\x1b[J"),fflush(stdout))

int dev_vcs=-1;
int wid,hei,vcsize;

/*
 * These two functions are only involved in the user interface:
 * dump and restore the screen to keep things up to date.
 */

unsigned short clear_sel_args[6]={0, 0,0, 0,0, 4};
unsigned char *clear_sel_arg= (unsigned char *)clear_sel_args+1;


static inline int scrdump(char *buf)
{
   clear_sel_arg[0]=2;  /* clear_selection */
   ioctl(fileno(stdin),TIOCLINUX,clear_sel_arg);
   lseek (dev_vcs, 0, SEEK_SET);
   return read(dev_vcs,buf,vcsize);
}

static inline int scrrestore(char *buf)
{
   clear_sel_arg[0]=2;  /* clear_selection */
   ioctl(fileno(stdin),TIOCLINUX,clear_sel_arg);
   lseek (dev_vcs, 0, SEEK_SET);
   return write(dev_vcs,buf,vcsize);
}


/* I don't like curses, so I'm doing low level stuff here */
static void raw(void)
{
struct termios it;

tcgetattr(fileno(stdin),&it);
it.c_lflag &= ~(ICANON);
it.c_lflag &= ~(ECHO);
it.c_iflag &= ~(INPCK|ISTRIP|IXON);
it.c_oflag &= ~(OPOST);
it.c_cc[VMIN] = 1;
it.c_cc[VTIME] = 0;

tcsetattr(fileno(stdin),TCSANOW,&it);

}

static void noraw(void)
{
struct termios it;

tcgetattr(fileno(stdin),&it);
it.c_lflag |= ICANON;
it.c_lflag |= ECHO;
it.c_iflag |= IXON;
it.c_oflag |= OPOST;

tcsetattr(fileno(stdin),TCSANOW,&it);
}



/* this one is the signal handler */

void killed(int signo)
{
   CLEAR;
   fprintf(stderr,"hltest: killed by signal %i\r\n",signo);
   noraw();
   exit(0);
}

char *dumpbuf;
char *dumpbuf_clean;
#define DUMPCHAR(x,y) (dumpbuf+4+2*((y)*wid+(x)))
#define DUMPATTR(x,y) (dumpbuf+5+2*((y)*wid+(x)))

/*
 * This function draws one window on top of the others
 */

static inline int drawwin(Gpm_Roi *which)
{
char *curr;
char name[5];

#define GOTO(x,y)     (curr=DUMPCHAR(x,y))
#define PUTC(c,a)   (*(curr++)=(c),*(curr++)=a)
#define ULCORNER 0xc9
#define URCORNER 0xbb
#define LLCORNER 0xc8
#define LRCORNER 0xbc
#define HORLINE  0xcd
#define VERLINE  0xba
 
int attrib=((WinInfo *)which->clientdata)->color;
int i,j;

   /* top border */
   GOTO(which->xMin,which->yMin);
   PUTC(ULCORNER,attrib);
   for (i=which->xMin+1; i<which->xMax; i++)
   PUTC(HORLINE,attrib);
   PUTC(URCORNER,attrib);

   /* sides and items */
   for (j=which->yMin+1;j<which->yMax;j++)
   {
   GOTO(which->xMin,j);
   PUTC(VERLINE,attrib);
   for (i=which->xMin+1; i<which->xMax; i++)
     PUTC(' ',attrib);
   PUTC(VERLINE,attrib);
   }

   /* bottom border */
   GOTO(which->xMin,which->yMax);
   PUTC(LLCORNER,attrib);
   for (i=which->xMin+1; i<which->xMax; i++)
   PUTC(HORLINE,attrib);
   PUTC(LRCORNER,attrib);

   /* name */
   sprintf(name,"%3i",((WinInfo *)which->clientdata)->number);
   GOTO(which->xMin+1,which->yMin+1);
   for (i=0;name[i];i++)
   PUTC(name[i],attrib);
   return 0;
}

/* and finally, draw them all (recursive) */

int drawthemall(Gpm_Roi *this)
{
   if (this->next)
   drawthemall(this->next);
   else
   memcpy(dumpbuf,dumpbuf_clean,vcsize);
   
   return drawwin(this);
}

/*
 * This one writes all the messages in the right position
 */

int newmsg(int window, char *msg)
{
   static char *data=NULL;
   static char **strings;
   static int current, last;
   static time_t t;
   int i,j;

   /* first time, alloc material */
   if (!data)
   {
   data=malloc((LEFTWID-2)*(hei-2));
   strings=malloc(hei*sizeof(char *));
   if(!data || !strings) { perror("malloc()"); exit(1); }
   memset(data,' ',(LEFTWID-2)*(hei-2));
   for (i=0;i<hei-2;i++)
     strings[i]=data+(LEFTWID-2)*i, strings[i][19]='\0';
   current=0;
   }
  
   if (msg) /* if sth new */
   {
   time(&t);
   strftime(strings[current],15,"%M:%S ",localtime(&t));
   sprintf(strings[current]+strlen(strings[current]),"%i: ",window);
   strncat(strings[current],msg,LEFTWID-2-strlen(strings[current]));
   for (i=strlen(strings[current]);i<19;i++)
     strings[current][i]=' ';
   strings[current][i]=0;
   last=current; current++; current%=(hei-2);
   }
   else /* draw them all */
   for (i=1,j=current; j%=(hei-2), i<hei-2; i++,j++)
     printf("\x1B[%03i;%03iH%s",j+2,2,strings[j]);  /* y and x */

   /* write only the current one */
   printf("\x1B[%03i;%03iH%s\x1B[%03i;%03iH%s",
       last+2,2,strings[last],current+2,2,"                    ");
   fflush(stdout);
   return 0;
}

/*
 * Wrapper: refresh the whole screen
 */
static inline void dorefresh(void)
{
static unsigned short clear_sel_args[6]={0, 0,0, 0,0, 4};
static unsigned char *clear_sel_arg= (unsigned char *)clear_sel_args+1;

   drawthemall(gpm_roi);
   clear_sel_arg[0]=2;  /* clear_selection */
   ioctl(fileno(stdin),TIOCLINUX,clear_sel_arg);
   scrrestore(dumpbuf);
   newmsg(0,NULL);
}


/*============================================================ MOUSE-BEGIN */

/*
 * In this sample program, I use only one handler, which reports on the
 * screen any event it gets. The clientdata received is the one I pass to
 * Gpm_PushRoi on creation, and is used to differentiate them
 */
 
int handler(Gpm_Event *ePtr, void *clientdata)
{
WinInfo *info=clientdata;
int number=info->number;
int delta;

   if (ePtr->type&GPM_ENTER)
   { newmsg(number,"enter"); return 0; }
   if (ePtr->type&GPM_LEAVE)
   { newmsg(number,"leave"); return 0; }
   if (ePtr->type&GPM_MOVE)
   { newmsg(number,"move"); return 0; }

   if (ePtr->type&GPM_DOWN)
   {newmsg(number,"press"); return 0; }
   if (ePtr->type&GPM_DRAG)
   {
   newmsg(number,"drag");
   if (!(ePtr->buttons&GPM_B_LEFT)) return 0;

   /*
    * Implement window motion
    */
   delta=0;
   if (ePtr->dx < 0)       delta=max(ePtr->dx,LEFTWID-info->roi->xMin);
   else if (ePtr->dx > 0)  delta=min(ePtr->dx,wid-1 - info->roi->xMax);
   if (delta) { info->roi->xMin+=delta; info->roi->xMax+=delta; }

   delta=0;
   if (ePtr->dy < 0)       delta=max(ePtr->dy,0     - info->roi->yMin);
   else if (ePtr->dy > 0)  delta=min(ePtr->dy,hei-1 - info->roi->yMax);
   if (delta) { info->roi->yMin+=delta; info->roi->yMax+=delta; }
   Gpm_RaiseRoi(info->roi,NULL);
   dorefresh(); return 0;
   }

   newmsg(number,"release");

/*
 * "up" events can be double or triple,
 * moreover, they can happen out of the region
 */

   if (!(ePtr->type&GPM_SINGLE)) return 0;
   if (ePtr->x<0 || ePtr->y<0) return 0;
   if (ePtr->x>info->wid-1 || ePtr->y>info->hei-1) return 0;

/*
 * Ok, it is has been a double click, and it is within our area
 */

   switch(ePtr->buttons)
   {
   case GPM_B_LEFT: Gpm_RaiseRoi(info->roi,NULL); break;
   case GPM_B_MIDDLE: Gpm_LowerRoi(info->roi,NULL); break;
   case GPM_B_RIGHT:
     Gpm_PopRoi(info->roi); free(info); break;
   }
   dorefresh();
   return 0;
}


/*
 * the following function creates a window, with the four corners as
 * specified by the arguments.
 */
int wincreate(int x, int y, int X, int Y)
{
   static int winno=0;
   Gpm_Roi*roi;
   WinInfo *cldata;
   int tmp;

   if (X<x) tmp=X,X=x,x=tmp;
   if (Y<y) tmp=Y,Y=y,y=tmp;
   if (X-x<5) X=x+4;
   if (Y-y<3) Y=y+2;
   if (X>=wid) X=wid-1;
   if (Y>=hei) Y=hei-1;
   if (x<LEFTWID) x=LEFTWID;
   if (y<0) y=0;

   cldata=malloc(sizeof(WinInfo));
   roi=Gpm_PushRoi(x,y,X,Y,
              GPM_DOWN|GPM_UP|GPM_DRAG|GPM_ENTER|GPM_LEAVE,
              handler,cldata);
   if (!cldata || !roi) { perror("malloc()"); exit(1); }

   /* don't get modifiers */
   roi->maxMod=0;

   /* put a backpointer */
   cldata->roi=roi;

   /* init the window */
   winno++;
   cldata->number=winno;
   cldata->color=colors[winno%NR_COLORS];
   cldata->wid=X-x+1;
   cldata->hei=Y-y+1;
   dorefresh();
return 0;
}


/*
 * This extra handler is only used for the background and left bar
 */

int xhandler(Gpm_Event *ePtr, void *clientdata)
{
static int x=0,y=0;
int back=0;
char msg[32];

msg[0]='\0';
if (ePtr->type==GPM_MOVE) return 0;
if (!clientdata)
   strcpy(msg,"bg"),back++;

if (ePtr->type&GPM_DOWN)
   {
   strcat(msg," down");
   if (back) x=ePtr->x,y=ePtr->y;
   }
if (ePtr->type&GPM_UP)
   {
   strcat(msg," up");
   if (back) wincreate(x,y,ePtr->x,ePtr->y);
   }

newmsg(0,msg);
/* GPM_DRAWPOINTER(ePtr); useless, gpm_visiblepointer is set to 1 BUG */
return 0;
}

/*============================================================ MOUSE-END */

/*
 * This function is not interesting, it is only involved in setting up
 * things. The real work has already been done.
 */

int main(int argc, char **argv)
{
   Gpm_Connect conn;
   char *s, t[4];
   char devname[32]; /* very secure buffer ... */
   int c;
   int vc;
   struct winsize win;

   if (argc>1) {
      fprintf(stderr,"%s: This program is meant to be interactive, "
                     "no option is needed\n",argv[0]);
      exit(1);
   }

   if (!(s=ttyname(fileno(stdin)))) {
      perror("stdin");
      exit(1);
   }

   /* retrieve everything */
   conn.eventMask=~0;
   conn.defaultMask=GPM_MOVE|GPM_HARD;
   conn.maxMod=~0;
   conn.minMod=0;
 
   if (sscanf(s,"/dev/tty%d%s",&vc,t)!=1) {
      if (sscanf(s,"/dev/vc/%d%s",&vc,t)!=1) {
         fprintf(stderr,"stdin: not a system console\n");
         exit(1);
      }
   }   

   /* open your dump/restore buffers */

   sprintf(devname,"/dev/vcs%i",vc);
   if ((dev_vcs=open(devname,O_RDWR))<0) {
      perror(devname);
      sprintf(devname,"/dev/vcc/%i",vc);
      if ((dev_vcs=open(devname,O_RDWR))<0) {
         perror(devname);
         exit(1);
      }
   }

   if (Gpm_Open(&conn,0) == -1) {
      fprintf(stderr,"%s: Can't open mouse connection\n",argv[0]);
      exit(1);
   }

   Gpm_Close(); /* don't be clubbered by events now */

   signal(SIGWINCH,killed); /* don't handle winchange */
   signal(SIGINT,killed);   /* control-C kills us */

   CLEAR;
 
   printf("\t\t\t  This program is a demonstration of the use of the\n"
       "\t\t\t  high level gpm library. It is a tiny application\n"
       "\t\t\t  not really useful, but you may be interested in\n"
       "\t\t\t  reading the source (as well as the docs).\n\n"
       "\t\t\tWorkings:\n"
       "\t\t\t  You can create windows by clicking on the background\n"
       "\t\t\t  and dragging out the desired size (minimun is 5x3).\n"
       "\t\t\t  The first 20 columns are reserved for messages.\n"
       "\t\t\t  there is always one invisible window on the backgruond\n"
       "\t\t\t  which gets shift-mouse events. All the others are only\n"
       "\t\t\t  sensitive to mouse-only events (the library leaves you\n"
       "\t\t\t  free about that, but this program makes things simple\n"
       "\n"
       "\t\t\t  The left button raises the window\n"
       "\t\t\t  The middle (if any) lowers the window\n"
       "\t\t\t  The right one destroys the window\n\n"
       "\t\t\t  Keyboard focus is changed by mouse motion\n"
       "\t\t\t  Control-C kills the program\n\n"
       "\t\t\tEnjoy\n\t\t\t      The Author\n\n"
       );

   /* Ok, now retrieve window info, go to raw mode, and start it all */

   ioctl(fileno(stdin), TIOCGWINSZ, &win);
   wid=win.ws_col; hei=win.ws_row;
   vcsize=hei*wid*2+4;
   dumpbuf=malloc(vcsize); dumpbuf_clean=malloc(vcsize);
   if (!dumpbuf || ! dumpbuf_clean)
   { perror("malloc()"); exit(1); }
   scrdump(dumpbuf_clean);

   gpm_zerobased=1; gpm_visiblepointer=1;
   if (Gpm_Open(&conn,0)==-1)
   { fprintf(stderr,"%s: Can't open mouse connection\n",argv[0]); exit(1); }

   /* Prepare the leftish roi, and the catch-all on the background */
   left_cldata.color=0x07; left_cldata.number=0;
   left_cldata.hei=hei; left_cldata.wid=LEFTWID;
   Gpm_PushRoi(0,0,LEFTWID-1,hei-1,
           GPM_DOWN|GPM_DRAG|GPM_UP|GPM_ENTER|GPM_LEAVE,
           xhandler,&left_cldata);
   gpm_roi_handler=xhandler;
   gpm_roi_data=NULL;

   raw();
   newmsg(0,NULL); /* init data structures */
   while((c=Gpm_Getchar())!=EOF) {
      char s[32];
      Gpm_Roi *roi;

   roi=gpm_current_roi;
   sprintf(s,"0x%x",c);
   if (c>0x1f && c<0x7f)
     sprintf(s+strlen(s)," ('%c')",c);
   newmsg(roi ? ((WinInfo *)roi->clientdata)->number : 0,s);
   }

   noraw();
   exit(0);
}
