/*
 * liblow.c - client library - low level (gpm)
 *
 * Copyright 1994,1995   rubini@linux.it (Alessandro Rubini)
 * Copyright (C) 1998    Ian Zimmerman <itz@rahul.net>
 * Copyright 2001        Nico Schottelius (nico-gpm@schottelius.org)
 * Copyright 2004        Dmitry Torokhov <dtor@mail.ru>
 *
 * xterm management is mostly by jtklehto@stekt.oulu.fi (Janne Kukonlehto)
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
#include <ctype.h>
#include <string.h>        /* strncmp */
#include <unistd.h>        /* select(); */
#include <errno.h>
#include <sys/time.h>      /* timeval */
#include <sys/types.h>     /* socket() */
#include <sys/socket.h>    /* socket() */
#include <sys/un.h>        /* struct sockaddr_un */
#include <sys/fcntl.h>     /* O_RDONLY */
#include <sys/stat.h>      /* stat() */

#ifdef  SIGTSTP         /* true if BSD system */
#include <sys/file.h>
#include <sys/ioctl.h>
#endif

#include <signal.h>
#include <linux/vt.h>      /* VT_GETSTATE */
#include <sys/kd.h>        /* KDGETMODE */
#include <termios.h>       /* winsize */

#include "headers/gpmInt.h"
#include "headers/message.h"

#ifndef min
#define min(a,b) ((a)<(b) ? (a) : (b))
#define max(a,b) ((a)>(b) ? (a) : (b))
#endif

/*....................................... Stack struct */
typedef struct Gpm_Stst {
   Gpm_Connect info;
   struct Gpm_Stst *next;
} Gpm_Stst;

/*....................................... Global variables */
int gpm_flag;   /* Open connections count */
int gpm_tried;
int gpm_fd = -1;
int gpm_hflag;
Gpm_Stst *gpm_stack;
struct timeval gpm_timeout = {10, 0};
Gpm_Handler *gpm_handler;
void *gpm_data;
int gpm_zerobased;
int gpm_visiblepointer;
int gpm_mx, gpm_my; /* max x and y (1-based) to fit margins */

unsigned char    _gpm_buf[6 * sizeof(short)];
unsigned short * _gpm_arg = (unsigned short *)_gpm_buf + 1;

int gpm_consolefd = -1;  /* used to invoke ioctl() */
int gpm_morekeys;

int gpm_convert_event(unsigned char *mdata, Gpm_Event *ePtr);

/*----------------------------------------------------------------------------*
 * nice description
 *----------------------------------------------------------------------------*/
static inline int putdata(int where, Gpm_Connect *what)
{
#ifdef GPM_USE_MAGIC
   static int magic = GPM_MAGIC;

   if (write(where, &magic, sizeof(int)) != sizeof(int)) {
      gpm_report(GPM_PR_ERR, GPM_MESS_WRITE_ERR, strerror(errno));
      return -1;
   }
#endif
   if (write(where,what,sizeof(Gpm_Connect))!=sizeof(Gpm_Connect)) {
      gpm_report(GPM_PR_ERR, GPM_MESS_WRITE_ERR, strerror(errno));
      return -1;
   }
   return 0;
}

static void get_screen_dimensions(void)
{
   struct winsize win;

   if (ioctl(gpm_consolefd, TIOCGWINSZ, &win) == 0) {
      if (!win.ws_col || !win.ws_row) {
         win.ws_col = 80;
         win.ws_row = 25;
      }
      gpm_mx = win.ws_col - gpm_zerobased;
      gpm_my = win.ws_row - gpm_zerobased;
   }
}

/*----------------------------------------------------------------------------*
 * nice description
 *----------------------------------------------------------------------------*/
#if defined(SIGWINCH)

static struct sigaction gpm_saved_winch_hook; /* Old SIGWINCH handler. */
static void gpm_winch_hook (int signum)
{
   if (SIG_IGN != gpm_saved_winch_hook.sa_handler &&
       SIG_DFL != gpm_saved_winch_hook.sa_handler) {
      gpm_saved_winch_hook.sa_handler(signum);
   }

   get_screen_dimensions();
}

static void install_winch_hook(void)
{
   struct sigaction sa;

   sigemptyset(&sa.sa_mask);
   sa.sa_handler = gpm_winch_hook;
   sa.sa_flags = 0;
   sigaction(SIGWINCH, &sa, &gpm_saved_winch_hook);
}

static void remove_winch_hook(void)
{
   sigaction(SIGWINCH, &gpm_saved_winch_hook, 0);
}

#else
#define install_winch_hook()  /* nothing */
#define remove_winch_hook()   /* nothing */
#endif /* SIGWINCH */

/*----------------------------------------------------------------------------*
 * nice description
 *----------------------------------------------------------------------------*/
#if defined(SIGTSTP)

static struct sigaction gpm_saved_suspend_hook;

static void gpm_suspend_hook(int signum)
{
   Gpm_Connect gpm_connect;
   sigset_t old_sigset;
   sigset_t new_sigset;
   struct sigaction sa;
   int success;

   sigemptyset(&new_sigset);
   sigaddset(&new_sigset, SIGTSTP);
   sigprocmask(SIG_BLOCK, &new_sigset, &old_sigset);

   /* Open a completely transparent gpm connection */
   gpm_connect.eventMask = 0;
   gpm_connect.defaultMask = ~0;
   gpm_connect.minMod = ~0;
   gpm_connect.maxMod = 0;
   /* cannot do this under xterm, tough */
   success = (Gpm_Open(&gpm_connect, 0) >= 0);

   /* take the default action, whatever it is (probably a stop :) */
   sigprocmask(SIG_SETMASK, &old_sigset, 0);
   sigaction(SIGTSTP, &gpm_saved_suspend_hook, 0);
   kill(getpid(), SIGTSTP);

   /* in bardo here */

   /* Reincarnation. Prepare for another death early. */
   sigemptyset(&sa.sa_mask);
   sa.sa_handler = gpm_suspend_hook;
   sa.sa_flags = SA_NOMASK;
   sigaction(SIGTSTP, &sa, 0);

   /* Pop the gpm stack by closing the useless connection */
   /* but do it only when we know we opened one.. */
   if (success)
      Gpm_Close();
}

static void install_suspend_hook(void)
{
   struct sigaction sa;

   sigemptyset(&sa.sa_mask);
   sa.sa_handler = SIG_IGN;
   sigaction(SIGTSTP, &sa, &gpm_saved_suspend_hook);

   /* if signal was originally ignored, job control is not supported */
   if (gpm_saved_suspend_hook.sa_handler != SIG_IGN) {
      sa.sa_flags = SA_NOMASK;
      sa.sa_handler = gpm_suspend_hook;
      sigaction(SIGTSTP, &sa, 0);
   }
}

static void remove_suspend_hook(void)
{
   sigaction(SIGTSTP, &gpm_saved_suspend_hook, 0);
}

#else
#define install_suspend_hook()   /* nothing */
#define remove_suspend_hook()    /* nothing */
#endif /* SIGTSTP */

static int open_server_socket(void)
{
   struct sockaddr_un addr;
   char *sock_name = NULL;
   int fd;

   if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
      gpm_report(GPM_PR_ERR, GPM_MESS_SOCKET, strerror(errno));
      return -1;
   }

#ifndef SO_PEERCRED
   memset(&addr, 0, sizeof(addr));
   addr.sun_family = AF_UNIX;
   if (!(sock_name = tempnam(0, "gpm"))) {
      gpm_report(GPM_PR_ERR, GPM_MESS_TEMPNAM, strerror(errno));
      goto err;
   }
   strncpy(addr.sun_path, sock_name, sizeof(addr.sun_path));
   if (bind(fd, (struct sockaddr *)&addr,
            sizeof(addr.sun_family) + strlen(addr.sun_path)) == -1) {
      gpm_report(GPM_PR_ERR, GPM_MESS_DOUBLE_S, sock_name, strerror(errno));
      goto err;
   }
#endif

   memset(&addr, 0, sizeof(addr));
   addr.sun_family = AF_UNIX;
   strcpy(addr.sun_path, GPM_NODE_CTL);

   if (connect(fd, (struct sockaddr *)&addr,
               sizeof(addr.sun_family) + strlen(GPM_NODE_CTL)) < 0) {
      gpm_report(GPM_PR_INFO, GPM_MESS_DOUBLE_S, GPM_NODE_CTL, strerror(errno));
      goto err;
   }

   return fd;

 err:
   close(fd);
   if (sock_name) {
      unlink(sock_name);
      free(sock_name);
   }

   return -1;
}

static int open_server_device(void)
{
   struct stat stbuf;
   int fd;

   /*
    * Well, try to open a chr device called /dev/gpmctl. This should
    * be forward-compatible with a kernel server
    */
   if ((fd = open(GPM_NODE_DEV, O_RDWR)) == -1) {
      gpm_report(GPM_PR_ERR, GPM_MESS_DOUBLE_S, GPM_NODE_DEV, strerror(errno));
      return -1;
   }

   if (fstat(fd, &stbuf) == -1 || (stbuf.st_mode & S_IFMT) != S_IFCHR) {
      close(fd);
      return -1;
   }

   return fd;
}


/*----------------------------------------------------------------------------*
 * nice description
 *----------------------------------------------------------------------------*/
int Gpm_Open(Gpm_Connect *conn, int flag)
{
   static char *console_name;
   static int console_queried;

   char *tty = NULL, *tty_str = NULL;
   char *term;
   Gpm_Stst *new;

   /*....................................... First of all, check xterm */

   if ((term = getenv("TERM")) && !strncmp(term, "xterm", 5)) {
      gpm_fd = -2;
      GPM_XTERM_ON;
      gpm_flag = 1;
      return gpm_fd;
   }

   /*....................................... Not a xterm, go on */

   /* check whether console is accesible */
   if (!console_queried) {
      console_name = Gpm_get_console();
      console_queried = 1;
   }

   /*
    * If console is not set just bail out - GPM is not available unless
    * client is running on local console
    */
   if (!console_name)
      return -1;

   /*
    * So I chose to use the current tty, instead of /dev/console, which
    * has permission problems. (I am fool, and my console is
    * readable/writeable by everybody.
    *
    * However, making this piece of code work has been a real hassle.
    */

   if (!gpm_flag && gpm_tried)
      return -1;

   gpm_tried = 1; /* do or die */

   if ((new = malloc(sizeof(Gpm_Stst))) == NULL) {
      gpm_report(GPM_PR_ERR, "Not enough memory for a new connection");
      return -1;
   }

   conn->pid = getpid(); /* fill obvious values */

   if (gpm_stack) {
      conn->vc = new->next->info.vc; /* inherit */
   } else {
      conn->vc = 0;                 /* default handler */
      if (flag > 0) {  /* forced vc number */
         conn->vc = flag;
         tty = tty_str = malloc(strlen(console_name) + Gpm_cnt_digits(flag));
         if (!tty) {
            gpm_report(GPM_PR_ERR, "Not enough memory for tty name");
            goto err;
         }
         memcpy(tty, console_name, strlen(console_name) - 1);
         sprintf(&tty[strlen(console_name) - 1], "%i", flag);
      } else { /* use your current vc */
         if (isatty(0)) tty = ttyname(0);             /* stdin */
         if (!tty && isatty(1)) tty = ttyname(1);     /* stdout */
         if (!tty && isatty(2)) tty = ttyname(2);     /* stderr */
         if (!tty) {
            gpm_report(GPM_PR_ERR, "Checking tty name failed");
            goto err;
         }

         /* Make sure that we actually running aon console */
         if (strncmp(tty, console_name, strlen(console_name) - 1) ||
             !isdigit(tty[strlen(console_name) - 1])) {
            gpm_report(GPM_PR_DEBUG, "Not running on local console");
            goto err;
         }

         conn->vc = atoi(&tty[strlen(console_name) - 1]);
      }

      if (gpm_consolefd == -1) {
         if ((gpm_consolefd = open(tty, O_WRONLY)) < 0) {
            gpm_report(GPM_PR_ERR, GPM_MESS_DOUBLE_S, tty, strerror(errno));
            goto err;
         }
      }
   }

   new->info = *conn;

   get_screen_dimensions();

   /*....................................... Connect to the control socket */

   if (!gpm_flag) {
      if ((gpm_fd = open_server_socket()) < 0)
         if ((gpm_fd = open_server_device()) < 0)
            goto err;
   }

   new->next = gpm_stack;
   gpm_stack = new;
   gpm_flag++;

   /*....................................... Put your data */
   if (putdata(gpm_fd, conn) != -1) {
      install_winch_hook();
      if (gpm_flag == 1) /* first open only */
         install_suspend_hook();
   }

   return gpm_fd;

  /*....................................... Error: free all memory */
 err:
   free(new);
   if (tty_str) free(tty_str);

   return -1;
}

/*-------------------------------------------------------------------*/
int Gpm_Close(void)
{
   Gpm_Stst *next;

   gpm_tried = 0; /* reset the error flag for next time */

   if (gpm_fd == -2) /* xterm */
      GPM_XTERM_OFF;
   else {            /* linux */
      if (!gpm_flag)
         return 0;
      next = gpm_stack->next;
      free(gpm_stack);
      gpm_stack = next;
      if (next)
         putdata(gpm_fd, &next->info);

      if (--gpm_flag)
         return -1;
   }

   if (gpm_fd >= 0)
      close(gpm_fd);
   gpm_fd = -1;

   remove_suspend_hook();
   remove_winch_hook();

   if (gpm_consolefd >= 0)
      close(gpm_consolefd);
   gpm_consolefd = -1;

   return 0;
}

/*-------------------------------------------------------------------*/
int Gpm_GetEvent(Gpm_Event *event)
{
   int count;
   MAGIC_P((int magic));

   if (!gpm_flag)
      return 0;

#ifdef GPM_USE_MAGIC
   if ((count = read(gpm_fd, &magic, sizeof(int))) != sizeof(int)) {
      if (count == 0) {
          gpm_report(GPM_PR_INFO,"Warning: closing connection");
          Gpm_Close();
          return 0;
      }
      gpm_report(GPM_PR_INFO, "Read too few bytes (%i) at %s:%d",
                 count, __FILE__, __LINE__);
      return -1;
   }
#endif

   if ((count = read(gpm_fd, event, sizeof(Gpm_Event))) != sizeof(Gpm_Event)) {
#ifndef GPM_USE_MAGIC
      if (count == 0) {
          gpm_report(GPM_PR_INFO,"Warning: closing connection");
          Gpm_Close();
          return 0;
      }
#endif
      /*
       * avoid to send the message if there is no data; sometimes it makes
       * sense to poll the mouse descriptor any now an then using a
       * non-blocking descriptor
       */
      if (count != -1 || errno != EAGAIN)
	      gpm_report(GPM_PR_INFO, "Read too few bytes (%i) at %s:%d",
			           count, __FILE__, __LINE__);
      return -1;
   }

   event->x -= gpm_zerobased;
   event->y -= gpm_zerobased;

   return 1;
}


#define MAXNBPREVCHAR 4         /* I don't think more is usefull, JD */
static int n_prevchar, prevchar[MAXNBPREVCHAR];

/*-------------------------------------------------------------------*/
int Gpm_CharsQueued ()
{
   return n_prevchar;
}

/*-------------------------------------------------------------------*/
int Gpm_WaitForData(int fd)
{
   struct timeval tmo = { SELECT_TIME, 0 };
   static fd_set selSet;
   int max_fd = (gpm_fd > fd) ? gpm_fd : fd;

   do {
      FD_ZERO(&selSet);
      FD_SET(fd, &selSet);
      if (gpm_fd >= 0)
         FD_SET(gpm_fd, &selSet);

   } while (select(max_fd + 1, &selSet, NULL, NULL, &tmo) <= 0);

   return FD_ISSET(fd, &selSet) ? fd : gpm_fd;
}

/*-------------------------------------------------------------------*/
int Gpm_WaitMoreData(int fd)
{
   struct timeval tmo = { 0, 100 * 1000 };   /* 100 msecs */
   static fd_set selSet;
   int rc;

   FD_ZERO(&selSet);
   FD_SET(fd, &selSet);

   while ((rc = select(fd + 1, &selSet, NULL, NULL, &tmo)) == EINTR)
      /* empty */;

   return rc > 0;
}


/*-------------------------------------------------------------------*/
int Gpm_Getc(FILE *f)
{
   static Gpm_Event ev;
   static int nbuf_set;

   int fd = fileno(f);
   char mdata[4];
   int c, result;

   /* Hmm... I must be sure it is unbuffered */
   if (!nbuf_set) {
      setvbuf(f, NULL, _IONBF, 0);
      nbuf_set = 1;
   }

   if (!gpm_flag)
      return getc(f);

   /* If the handler asked to provide more keys, give them back */
   if (gpm_morekeys)
      return (*gpm_handler)(&ev, gpm_data);

   gpm_hflag = 0;

   if (n_prevchar)  /* if there are some consumed char ... */
      return prevchar[--n_prevchar];

   while (1) {
      if (gpm_fd >= 0 && gpm_visiblepointer)
         GPM_DRAWPOINTER(&ev);

      if (Gpm_WaitForData(fd) != fd) {
         /* GPM is available (running on console), iget event from it */

         if (!Gpm_GetEvent(&ev))
            continue;

      } else if (gpm_fd == -2) {
         /* Running in Xterm, is it a mouse event or regular data? */

         if ((c = fgetc(f)) != 0x1b)
            return c;

         /* escape: go on */
         if (!Gpm_WaitMoreData(fd))
            return c;

         if ((c = fgetc(f)) != '[') {
            prevchar[n_prevchar++] = c;
            return 0x1B;
         }

         /* '[': go on */
         if (!Gpm_WaitMoreData(fd)) {
            prevchar[n_prevchar++] = c;
            return 0x1B;
         }

         if ((c = fgetc(f)) != 'M') { /* NOTICE: prevchar is a lifo !*/
            prevchar[n_prevchar++] = c;
            prevchar[n_prevchar++] = '[';
            return 0x1B;
         }

         /* now, it surely is a mouse event */
         for (c = 0; c < 3; c++)
            mdata[c] = fgetc(f);
         gpm_convert_event(mdata, &ev);

      } else {
         /* Not running in Xterm and not GPM event, just read the data */
         return fgetc(f);
      }

      /* Ok, we have a new event, handle it */
      if (gpm_handler && (result = gpm_handler(&ev, gpm_data))) {
         gpm_hflag = 1;
         return result;
      }
   }
}

/*-------------------------------------------------------------------*/
int Gpm_Repeat(int msec)
{
   struct timeval to = {0, 1000 * msec};
   fd_set selSet;
   int fd = gpm_fd >= 0 ? gpm_fd : 0;  /* either the socket or stdin */

   FD_ZERO(&selSet);
   FD_SET(fd, &selSet);
   return select(fd + 1, &selSet, NULL, NULL, &to) == 0;
}

/*-------------------------------------------------------------------*/
int Gpm_FitValuesM(int *x, int *y, int margin)
{
   switch (margin) {
      case -1:
         *x = min(max(*x, !gpm_zerobased), gpm_mx);
         *y = min(max(*y, !gpm_zerobased), gpm_my);
         break;

      case GPM_TOP: (*y)++; break;
      case GPM_BOT: (*y)--; break;
      case GPM_RGT: (*x)--; break;
      case GPM_LFT: (*x)++; break;
   }
   return 0;
}

/*-------------------------------------------------------------------*/
int gpm_convert_event(unsigned char *mdata, Gpm_Event *ePtr)
{
   static struct timeval tv1, tv2;
   static int clicks;
   int c;

#define GET_TIME(tv) (gettimeofday(&tv, (struct timezone *)NULL))
#define DIF_TIME(t1,t2) ((t2.tv_sec -t1.tv_sec) *1000+ \
                         (t2.tv_usec-t1.tv_usec)/1000)


   /* Variable btn has following meaning: */
   c = mdata[0] - 32; /* 0="1-down", 1="2-down", 2="3-down", 3="up" */

   if (c == 3) {
      ePtr->type = GPM_UP | (GPM_SINGLE << clicks);
      /* ePtr->buttons = 0; */ /* no, keep info from press event */
      GET_TIME(tv1);
      clicks = 0;
   } else {
      ePtr->type = GPM_DOWN;
      GET_TIME(tv2);
      if (tv1.tv_sec && (DIF_TIME(tv1, tv2) < 250)) {
         /* 250ms for double click */
         clicks++; clicks %= 3;
      } else
         clicks = 0;

      switch (c) {
         case 0: ePtr->buttons = GPM_B_LEFT;   break;
         case 1: ePtr->buttons = GPM_B_MIDDLE; break;
         case 2: ePtr->buttons = GPM_B_RIGHT;  break;
         default:    /* Nothing */             break;
      }
   }
   /* Coordinates are 33-based */
   /* Transform them to 1-based */
   ePtr->x = mdata[1] - 32 - gpm_zerobased;
   ePtr->y = mdata[2] - 32 - gpm_zerobased;
   return 0;
}

/* Local Variables: */
/* c-indent-level: 3 */
/* End: */
