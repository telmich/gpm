/*
 * mouse-test.c 
 *
 * Copyright 1995   rubini@linux.it (Alessandro Rubini)
 * Copyright (C) 1998 Ian Zimmerman <itz@rahul.net>
 * Copyright (C) 2002 Nico Schottelius <nico@schottelius.org>
 *
 * Tue,  5 Jan 1999 23:15:23 +0000, modified by James Troup <james@nocrew.org>:
 * (main): exclude devices with a minor number of 130 from
 * the device probe to avoid causing spontaneous reboots on machines
 * where watchdog is used.  Reported by Jim Studt <jim@federated.com>
 * [Debian bug report #22602]
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

/* This code is horrible. Browse it at your risk */
/* hmm..read worse code before :) -nicos */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <termios.h>

#include "headers/message.h" /* to print */
#include "headers/gpmInt.h" /* to get mouse types */

#ifndef min
#define min(a,b) ((a)>(b)?(b):(a))
#define max(a,b) ((a)>(b)?(a):(b))
#endif


/* this material is needed to pass options to mice.c */
struct mouse_features mymouse = {
   DEF_TYPE, DEF_DEV, DEF_SEQUENCE,
   DEF_BAUD, DEF_SAMPLE, DEF_DELTA, DEF_ACCEL, DEF_SCALE, DEF_SCALE /*scaley*/,
   DEF_TIME, DEF_CLUSTER, DEF_THREE, DEF_GLIDEPOINT_TAP,
   (char *)NULL /* extra */,
   (Gpm_Type *)NULL,
   -1 /* fd */
};

/* and this is a workaroud */
struct winsize win;

struct mouse_features *which_mouse=&mymouse;

char *progname;
char *consolename;
int devcount=0;
int typecount=0;

struct item {
   Gpm_Type *this;
   struct item *next;
};

struct device {
   char *name;
   int fd;
   struct device *next;
};  

static int message(void)
{
printf("This program is designed to help you in detecting what type your\n"
   "mouse is. Please follow the instructions of this program. If you're\n"
   "bored before it is done, you can always press your 'Interrupt' key\n"
   "(usually Ctrl-C)\n\n");
printf("\t *** Remember: don't run any software which reads the mouse device\n"
   "\t *** while making this test. This includes \"gpm\","
   "\"selection\", \"X\"\n\n");
printf("Note that this program is by no means complete, and its main role is\n"
   "to detect how does the middle button work on serial mice\n");
return 0;
}

/* I don't like curses, so I'm doing low level stuff here */

#define GOTOXY(f,x,y)   fprintf(f,"\x1B[%03i;%03iH",y,x)

/*----------------------------------------------------------------------------- 
   Place the description here.
 -----------------------------------------------------------------------------*/
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

/*----------------------------------------------------------------------------- 
   Place the description here.
 -----------------------------------------------------------------------------*/
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

/*----------------------------------------------------------------------------- 
   Place the description here.
 -----------------------------------------------------------------------------*/
void killed(int signo)
{
   fprintf(stderr,"mouse-test: killed by signal %i\r\n",signo);
   noraw();
   exit(0);
}

/*----------------------------------------------------------------------------- 
   Place the description here.
 -----------------------------------------------------------------------------*/
struct device **gpm_makedev(struct device **current, char *name)
{
   int fd; int modes;
   if ((fd=open(name,O_RDWR|O_NONBLOCK))==-1) {
      perror(name);
      return current;
   }
   modes = fcntl(fd, F_GETFL);
   if (0 > fcntl(fd, F_SETFL, modes & ~O_NONBLOCK)) {
      close(fd);
      perror(name);
      return current;
   }

   *current=malloc(sizeof(struct device));
   if (!*current) gpm_report(GPM_PR_OOPS,"malloc()");
   (*current)->name=strdup(name);
   if (!(*current)->name) gpm_report(GPM_PR_OOPS,"malloc()");
   (*current)->fd=fd;
   (*current)->next=NULL;
   devcount++;
   return &((*current)->next);
}

Gpm_Type *(*I_serial)(int fd, unsigned short flags, struct Gpm_Type *type,
		      int argc, char **argv);


/*----------------------------------------------------------------------------- 
   Place the description here.
 -----------------------------------------------------------------------------*/
int mousereopen(int oldfd, char *name, Gpm_Type *type)
{
   int fd;
   if (!type) type=mice+1; /* ms */
   close(oldfd);
   usleep(100000);
   fd=open(name,O_RDWR);
   if (fd < 0) gpm_report(GPM_PR_OOPS,name);
   (*I_serial)(fd,type->flags,type,1,&type->name); /* ms initialization */
   return fd;
}

int noneofthem(void)
{
   noraw();
   printf("\n\nSomething went wrong, I didn't manage to detect your"
	 "protocol\n\nFeel free to report your problems to the author\n");
   exit(1);
}

#define CHECKFAIL(count)   ((count)==0 && noneofthem())

/***************************************
 * This is the most useful function in
 * the program: it build an array
 * of data characters in order to run
 * several protocols on the returned
 * data.
 */

int eventlist(int fd, char *buff, int buflen, int test, int readstep)
{
   fd_set selSet, readySet;
   struct timeval to;
   int active=0;
   int pending;
   int got=0;

   FD_ZERO(&readySet);
   FD_SET(fd,&readySet);
   FD_SET(fileno(stdin),&readySet);
   to.tv_sec=to.tv_usec=0;

   switch(test) {
      case GPM_B_LEFT:
         printf("\r\nNow please press and release several times\r\n"
	         "the left button of your mouse,"
	         " *** trying not to move it ***\r\n\r\n");
         break;
    
      case GPM_B_MIDDLE:
         printf("\r\nNow please press and release several times\r\n"
	         "the middle button of your mouse,"
	         " *** trying not to move it ***\r\n\r\n");
         break;

      case 0:
         printf("\r\nNow please move the mouse around, without pressing any\r\n"
	         "button. Move it both quickly and slowly\r\n\r\n");
         break;

      default:
         printf("Unknown test to perform\r\n");
         return -1;
   }
  
   printf("Press any key when you've done enough\r\n");

   while(1) {
      selSet=readySet;
      if ((pending=select(fd+1,&selSet,NULL,NULL,&to)) < 0) continue;
      if (pending==0 && !active) { 
         active++;
         to.tv_sec=10;
         continue;
      }
      if (pending==0) return got; /* timeout */

      if (FD_ISSET(0,&selSet)) {
         getchar();
         if (active) return got;
      }
      if (FD_ISSET(fd,&selSet)) { 
         if (active) got+=read(fd,buff+got,readstep);
         else read(fd,buff,buflen);
      }
      if (got>buflen-readstep)
         readstep=buflen-got;
      if (readstep==0)
         got--,readstep++; /* overwrite last char forever */

      to.tv_sec=30;
   }
}

/*----------------------------------------------------------------------------- 
   Place the description here.
 -----------------------------------------------------------------------------*/
int main(int argc, char **argv)
{
   struct item *list=NULL;
   struct item **nextitem;
   struct device *devlist=NULL;
   struct device **nextdev;
   Gpm_Type *cursor;
   int i, mousefd;
   char *mousename;
#define BUFLEN 512
   char buf[BUFLEN];
   struct timeval timeout;
   fd_set checkSet;
   int pending, maxfd;
   int trial, readamount,packetsize,got;
   int baudtab[4]={1200,9600,4800,2400};
#define BAUD(i) (baudtab[(i)%4])
   consolename = Gpm_get_console();

   if (!isatty(fileno(stdin))) {
      fprintf(stderr,"%s: stdin: not a tty\n",argv[0]);
      exit(1);
   }

   progname=argv[0];
   message();

   /* init the list of possible devices */

   for (nextdev=&devlist, i=1; i<argc; i++)
      nextdev=gpm_makedev(nextdev,argv[i]);

   if (argc==1) { /* no cmdline, get all devices */
      FILE *f;
      char s[64];

      /* Skip 10, 130 (== watchdog) to avoid causing spontaneous reboots */
      f=popen("ls -l /dev/* | "
	    "awk '/^c/ && $5==\"10,\" && $6!=\"130\" || $NF ~ \"ttyS[0-3]$\" {print $NF}'",
	    "r");
      if (!f) gpm_report(GPM_PR_OOPS,"popen()");
      while (fgets(s,64,f)) {
         s[strlen(s)-1]='\0'; /* trim '\n' */
         nextdev=gpm_makedev(nextdev,s);
      }
      pclose(f);
   }

   if (devcount==0) {
      fprintf(stderr,"%s: no devices found\n",progname);
      exit(1);
   }

   I_serial=mice->init; /* the first one has I_serial */

   signal(SIGINT,killed);   /* control-C kills us */
   raw();

/*====================================== First of all, detect the device */

   trial=0;
   while (devcount > 1) {
      fd_set devSet, gotSet,savSet;
      struct device *cur;
      int gotthem;

      /* BUG */ /* Logitech initialization is not performed */

      opt_baud=BAUD(trial);
      printf("\r\nTrying with %i baud\r\n",opt_baud);
      trial++;

      FD_ZERO(&devSet); FD_ZERO(&gotSet);
      FD_SET(fileno(stdin),&devSet); maxfd=fileno(stdin);
      printf("\r\n The possible device nodes are:\r\n");
      for (nextdev=&devlist; *nextdev; nextdev=&((*nextdev)->next)) {
         printf("\t%s\r\n", (*nextdev)->name);
         FD_SET((*nextdev)->fd,&devSet);
         maxfd=max((*nextdev)->fd,maxfd);
         (*I_serial)((*nextdev)->fd,(mice+1)->flags,mice+1,
		  1, &(mice+1)->name); /* try ms mode */
      }

      savSet=devSet;
 
      printf("\r\n\r\nI've still %i devices which may be your mouse,\r\n",
	   devcount);
      printf("Please move the mouse. Press any key when done\r\n"
	         " (You can specify your device name on cmdline, in order to\r\n"
	         "  avoid this step\r\n. Different baud rates are tried at "
	         "different times\r\n");
    
      timeout.tv_sec=10; /* max test time */
      gotthem=0;
      while (1) { /* extract files from the list */
         devSet=savSet;
         if ((pending=select(maxfd+1,&devSet,NULL,NULL,&timeout)) < 0)
	         continue;
         if (pending==0 || FD_ISSET(fileno(stdin),&devSet)) { 
            getchar();
            break;
         }
         for (nextdev=&devlist; *nextdev; nextdev=&((*nextdev)->next))
	         if (FD_ISSET((*nextdev)->fd,&devSet)) {
	            gotthem++;
	            FD_CLR((*nextdev)->fd,&savSet);
	            FD_SET((*nextdev)->fd,&gotSet);
	         }
      }
      if (gotthem) for (nextdev=&devlist; *nextdev; /* nothing */ ) {
	      cur=*nextdev;
	      if (!FD_ISSET(cur->fd,&gotSet)) {
	         printf("removing \"%s\" from the list\r\n",cur->name);
	         *nextdev=cur->next;
	         close(cur->fd);
	         free(cur->name);
	         free(cur);
	         devcount--;
	      } else {
	         read(cur->fd,buf,80); /* flush */
	         nextdev=&(cur->next); /* follow list */
	      }
	   }
    
   } /* devcount>1 */

   mousefd=devlist->fd;
   mousename=devlist->name;
   free(devlist);
   printf("\r\nOk, so your mouse device is \"%s\"\r\n",mousename);

   /* now close and reopen it, complete with initialization */
   opt_baud=BAUD(0);
   mousefd=mousereopen(mousefd,mousename,NULL);
  
   FD_ZERO(&checkSet);
   FD_SET(mousefd,&checkSet);
   FD_SET(fileno(stdin),&checkSet);
   maxfd=max(mousefd,fileno(stdin));

/*====================================== Identify mouse type */
  
   /* init the list of possible types */

   printf("\r\n\r\nThe possible mouse types are:\r\n");
   for (nextitem=&list, cursor=mice; cursor->fun; cursor++) {
      if (cursor->absolute) continue;
      *nextitem=malloc(sizeof(struct item));
      if (!*nextitem) gpm_report(GPM_PR_OOPS,"malloc()");
      (*nextitem)->this=cursor; (*nextitem)->next=NULL;
      printf("\t%s",cursor->name);
      if (cursor->synonyms && cursor->synonyms[0])
         printf(" (%s)",cursor->synonyms);
      printf("\r\n");
      typecount++;
      nextitem=&((*nextitem)->next);
   }

/*====================================== Packet size - first step */

   printf("\r\nNow please press and release your left mouse button,\r\n"
	 "one time only\r\n\r\n");

   i=read(mousefd,buf,1);
   if (i==-1 && errno==EINVAL)
      readamount=3;
   else
      readamount=1;

   usleep(100000);

#define REMOVETYPE(cur,reason) \
   do { \
	 printf("\tRemoving type \"%s\" because of '%s'\r\n", \
		(cur)->this->name,reason); \
	 (*nextitem)=(cur)->next; \
	 free(cur); \
	 typecount--; \
       } while(0)


   for (nextitem=&list; *nextitem; /* nothing */) {
      struct item *cur=*nextitem;
    
      if (readamount!=cur->this->howmany)
         REMOVETYPE(cur,"different read() semantics");
      else
         nextitem=&(cur->next);
   }
   read(mousefd,buf,BUFLEN); /* flush */

/*====================================== Packet size - second step */

   printf("\r\nNow you will be asked to generate different kind of events,\r\n"
		 "in order to understand what protocol is your mouse speaking\r\n");

   if (readamount==1) readamount=10;
   /*
    * Otherwise, the packetsize is already known, but the different decoding
    * alghorithm allows for further refinement.
    */

   packetsize=1;
   trial=0;
   while (packetsize==1) {
      int success3=0,success5=0;
	
      opt_baud=BAUD(trial);
      printf("\tBaud rate is %i\r\n",opt_baud);
      mousefd=mousereopen(mousefd,mousename,NULL);
	
      printf("\r\n==> Detecting the packet size\r\n");
      got=eventlist(mousefd,buf,BUFLEN,GPM_B_LEFT,readamount);
	
      /* try three -- look at repeating arrays of 6 bytes */
      for (i=0;i<got-12;i++)
         if(!memcmp(buf+i,buf+i+6,6))
	   success3++;
	
      /* try five -- look at repeating arrays of 10 bytes */
      for (i=0;i<got-20;i++)
         if(!memcmp(buf+i,buf+i+10,10))
	   success5++;
	
      printf("matches for 3 bytes: %i   -- matches for 5 bytes: %i\r\n",
	   success3,success5);
      if (success3>5 && success5==0)
         packetsize=3; 
      if (success5>5 && success3==0)
         packetsize=5;
      if (packetsize==1)
         printf("\r\n ** Ambiguous, try again\r\n");
      trial++;
   }

/*====================================== Use that info to discard protocols */
  
   for (nextitem=&list; *nextitem; /* nothing */) {
      struct item *cur=*nextitem;
      int packetheads=0;
      int match=0;
      Gpm_Event event;

      if (packetsize!=cur->this->packetlen) {
         REMOVETYPE(cur,"different packet lenght");
         continue;
      }

      /* try to decode button press and release */
      for (i=0;i<got;i++) {
         if ( ((buf[i]  &(cur->this->proto)[0]) == (cur->this->proto)[1])
	      && ((buf[i+1]&(cur->this->proto)[2]) == (cur->this->proto)[3]) ) {
	         packetheads++;
	         if ((*(cur->this->fun))(&event,buf+i)==-1) {
               packetheads--;
               continue;
            }
	         i+=packetsize-1; /* skip packet */
	         if (event.dx || event.dy) continue;
	         if (event.buttons==GPM_B_LEFT)
	            match++;
	         else if (event.buttons)
	            match--;
	      }
      }
      if (match<=0) {
         REMOVETYPE(cur,"incorrect button detected");
         continue;
      }
      if (packetheads<got/(packetsize+2)) {
         REMOVETYPE(cur,"too few events got decoded");
         continue;
      }
      printf("** type '%s' still possible\r\n",cur->this->name);
      nextitem=&(cur->next);
   }

/*====================================== It's all done? */

/*
 * At this point, msc, would be unique (and 3-button aware) due to the
 * packet size; sun,mm and ps2 would be unique due to the different
 * representation of buttons (and they usually are not dual mode).
 */
  
   /* why checking and not using return value ??? */
   CHECKFAIL(typecount);
   if (typecount==1) {
      noraw();
      printf("\n\n\nWell, it seems like your mouse is already detected:\n"
      "it is on the device \"%s\", and speaks the protocol \"%s\"\n",
      mousename,list->this->name);
      exit(0);
   }

/*
 * If it is in the "ms" family, however, three possibilities are still
 * there, and all of them depend on the middle button
 */

   do {
      printf("\r\nYour mouse is one of the ms family, "
	   "but do you have the middle button (y/n)? ");
      putchar(i=getchar()); printf("\r\n");
   } while(i!='y' && i!='n');

   if (i=='n') {
      noraw();
      printf("\nThen, you should use the \"bare\" protocol on \"%s\"\n",
	   mousename);
      exit(0);
   }

/*
 * First trial: remove the "-t ms" extension if spurious buttons come in
 */

   got=eventlist(mousefd,buf,BUFLEN,0,readamount);
   pending=0;
   for (nextitem=&list; *nextitem; /* nothing */) {
      struct item *cur=*nextitem;
      Gpm_Event event;

      /* try to decode button press and release */
      for (i=0;i<got;i++) {
         if ( ((buf[i]  &(cur->this->proto)[0]) == (cur->this->proto)[1])
            && ((buf[i+1]&(cur->this->proto)[2]) == (cur->this->proto)[3]) ) {
               if ((*(cur->this->fun))(&event,buf+i)==-1) continue;
               i+=packetsize-1;
               if (event.buttons) pending--;
         }
      }
      if (pending<0) {
         REMOVETYPE(cur,"spurious button reported");
         continue;
      }
      printf("** type '%s' still possible\r\n",cur->this->name);
      nextitem=&(cur->next);
   }
   CHECKFAIL(typecount);

/*
 * Second trial: look if it is one of the two mman ways (In the second
 * test, extended ms is caught as well
 */

   printf("\r\n==> Looking for '-t mman'and enhanced ms\r\n");
   mousefd=mousereopen(mousefd,mousename, mice /* mman */);
   got=eventlist(mousefd,buf,BUFLEN,GPM_B_MIDDLE,readamount);

   /* if it uses the 4-byte protocol, find it in a rude way */
   for (pending=0,i=0;i<got-16;i++)
      if(!memcmp(buf+i,buf+i+8,8)) pending++;
   if (pending > 3) {
      noraw();
      printf("\nYour mouse seems to be a 'mman' one  on \"%s\" (%i matches)\n",
	   mousename,pending);
      exit(0);
   }

   pending=0;
   for (nextitem=&list; *nextitem; /* nothing */) {
      struct item *cur=*nextitem;
      Gpm_Event event;

      /* try to decode button press and release */
      for (i=0;i<got;i++) {
         if ( ((buf[i]  &(cur->this->proto)[0]) == (cur->this->proto)[1])
            && ((buf[i+1]&(cur->this->proto)[2]) == (cur->this->proto)[3]) ) {
               if ((*(cur->this->fun))(&event,buf+i)==-1) continue;
               i+=packetsize-1;
               if (event.buttons && event.buttons!=GPM_B_MIDDLE) pending--;
	            if (event.buttons==GPM_B_MIDDLE) pending++;
         }
      }
	   if (pending<0) {
         REMOVETYPE(cur,"spurious button reported");
         continue;
      }
	   if (pending>3) {
	      noraw();
	      printf("\nYour mouse seems to be a '%s' one on \"%s\" (%i matches)\n",
		   cur->this->name,mousename,pending);
	      exit(0);
	   }
	   printf("** type '%s' still possible\r\n",cur->this->name);
      nextitem=&(cur->next);
   }
   CHECKFAIL(typecount);
  
/*
 * Then, try to toggle dtr and rts
 */

   {
      int toggle[3]={TIOCM_DTR|TIOCM_RTS, TIOCM_DTR,TIOCM_RTS};
      char *tognames[3]={"both","dtr","rts"};
      char *Xtognames[3]={"'ClearDTR' and 'ClearRTS'","'ClearDTR'","'ClearRTS'"}; 
      int alllines,lines, index;

      ioctl(mousefd, TIOCMGET, &alllines);

      printf("\r\nSome mice change protocol to three-buttons-aware if some\r\n"
		   "\r\ncontrol lines are toggled after opening\r\n");
      for (index=0;index<3;index++) {
         mousereopen(mousefd,mousename,NULL);
         lines = alllines & ~toggle[index];
         ioctl(mousefd, TIOCMSET, &lines);
         printf("\r\n==> Trying with '-o %s'\r\n",tognames[index]);
         got=eventlist(mousefd,buf,BUFLEN,GPM_B_MIDDLE,readamount);
    
         /* if it uses the 5-byte protocol, find it in a rude way */
         for (pending=0,i=0;i<got-20;i++)
            if(!memcmp(buf+i,buf+i+10,10)) pending++;
         if (pending>3) {
            noraw();
            printf("\nYour mouse becomes a 3-buttons ('-t msc') one when\n"
	         "gpm gets '-o %s' on it command line, and X gets\n"
	         "%s in XF86Config\nThe device is \"%s\"",
	         tognames[index],Xtognames[index],mousename);
            exit(0);
         }
      }
   }
/*
 * still here? Then, the only resort is keeping the middle button
 * pressed while initializing the mouse
 */

   printf("\r\nYour damned device doesn't respond to anything\r\n"
	 "the only remaining possibility is to keep presses the middle\r\n"
	 "button _while_the_mouse_is_being_opened. This is the worst thing\r\n"
	 "ever thought after caps-lock-near-the-A\r\n");

   printf("\r\nNow please press the middle button, and then press any key\r\n"
	 "while keeping the button down. Wait to release it until the\r\n"
	 "next message.\r\n");
	
   getchar();

   got=eventlist(mousefd,buf,BUFLEN,GPM_B_MIDDLE,readamount);

   /* if it uses the 5-byte protocol, find it in a rude way */
   for (pending=0,i=0;i<got-20;i++)
      if(!memcmp(buf+i,buf+i+10,10)) pending++;
   if (pending>3) {
      noraw();
      printf("\nWorked. You should keep the button pressed every time the\n"
	   "computer boots, and run gpm in '-R' mode in order to ignore\n"
	   "such hassle when starting X\n\nStill better, but a better mouse\n"
	   "\nThe current mouse device is \"%s\"\n",mousename);

      exit(0);
   }
   noraw();
   printf("\nI'm lost. Can't tell you how to use your middle button\n");
   return 0;
}
