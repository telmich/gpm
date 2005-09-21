/*
 * general purpose mouse support for Linux
 *
 * Copyright (C) 1993        Andreq Haylett <ajh@gec-mrc.co.uk>
 * Copyright (C) 1994-1999   Alessandro Rubini <rubini@linux.it>
 * Copyright (C) 1998        Ian Zimmerman <itz@rahul.net>
 * Copyright (c) 2001,2002   Nico Schottelius <nico-gpm@schottelius.org>
 * Copyright (c) 2003        Dmitry Torokhov <dtor@mail.ru>
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
#include <limits.h>
#include <errno.h>
#include <unistd.h>        /* select(); */
#include <signal.h>        /* SIGPIPE */
#include <time.h>          /* time() */
#include <sys/fcntl.h>     /* O_RDONLY */
#include <sys/wait.h>      /* wait()   */
#include <sys/time.h>      /* timeval */

#include "headers/gpmInt.h"
#include "headers/message.h"
#include "headers/console.h"
#include "headers/selection.h"
#include "headers/client.h"

#ifndef max
#define max(a,b) ((a)>(b) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) ((a)<(b) ? (a) : (b))
#endif

#define NULL_SET ((fd_set *)NULL)
#define GET_TIME(tv) (gettimeofday(&tv, (struct timezone *)NULL))
#define DIF_TIME(t1,t2) ((t2.tv_sec - t1.tv_sec)*1000 + (t2.tv_usec - t1.tv_usec)/1000)


enum mouse_rslt { MOUSE_NO_DATA, MOUSE_DATA_OK, MOUSE_MORE_DATA };

extern int errno;

char *opt_special=NULL; /* special commands, like reboot or such */
struct repeater repeater;

static int console_resized; /* not really an option */

/*-------------------------------------------------------------------*/
static void gpm_killed(int signo)
{
   if (signo == SIGWINCH) {
      gpm_report(GPM_PR_WARN, GPM_MESS_RESIZING, option.progname, getpid());
      console_resized = 1;
   } else {
      if (signo == SIGUSR1)
         gpm_report(GPM_PR_WARN, GPM_MESS_KILLED_BY, option.progname, getpid(), option.progname);
      exit(0);
   }
}

/*-------------------------------------------------------------------
 * fetch the actual device data from the mouse device, dependent on
 * what Gpm_Type is being passed.
 *-------------------------------------------------------------------*/
static char *getMouseData(int fd, Gpm_Type *type, int text_mode)
{
   static unsigned char data[32]; /* quite a big margin :) */
   unsigned char *pdata;
   int len, togo;

   /*....................................... read and identify one byte */
   if (read(fd, data, type->howmany) != type->howmany) {
      gpm_report(GPM_PR_ERR, GPM_MESS_READ_FIRST, strerror(errno));
      return NULL;
   }

   if (!text_mode && repeater.fd != -1 && repeater.raw)
      write(repeater.fd, data, type->howmany);

   if ((data[0] & type->proto[0]) != type->proto[1]) {
      if (type->getextra == 1) {
         data[1] = GPM_EXTRA_MAGIC_1; data[2] = GPM_EXTRA_MAGIC_2;
         gpm_report(GPM_PR_DEBUG, GPM_EXTRA_DATA, data[0]);
         return data;
      }
      gpm_report(GPM_PR_DEBUG, GPM_MESS_PROT_ERR);
      return NULL;
   }

   /*....................................... read the rest */

   /*
    * well, this seems to work almost right with ps2 mice. However, I've never
    * tried ps2 with the original selection package, which called usleep()
    */
   if ((togo = type->packetlen - type->howmany)) { /* still to get */
      pdata = &data[type->howmany];
      do {
         if ((len = read(fd, pdata, togo)) == 0)
            break;
         if (!text_mode && repeater.fd != -1 && repeater.raw && len > 0)
            write(repeater.fd, pdata, len);
         pdata += len;
         togo -= len;
      } while (togo);
   }

   if (togo) {
      gpm_report(GPM_PR_ERR, GPM_MESS_READ_REST, strerror(errno));
      return NULL;
   }

   if ((data[1] & type->proto[2]) != type->proto[3]) {
      gpm_report(GPM_PR_INFO, GPM_MESS_SKIP_DATA);
      return NULL;
   }
   gpm_report(GPM_PR_DEBUG, GPM_MESS_DATA_4, data[0], data[1], data[2], data[3]);
   return data;
}

/*-------------------------------------------------------------------*/
void handle_console_resize(Gpm_Event *event)
{
   int old_x, old_y;
   struct micetab *mouse;

   old_x = console.max_x; old_y = console.max_y;
   refresh_console_size();
   if (!old_x) { /* first invocation, place the pointer in the middle */
      event->x = console.max_x / 2;
      event->y = console.max_y / 2;
   } else { /* keep the pointer in the same position where it was */
      event->x = event->x * console.max_x / old_x;
      event->y = event->y * console.max_y / old_y;
   }

   for (mouse = micelist; mouse; mouse = mouse->next) {
      /*
       * the following operation is based on the observation that 80x50
       * has square cells. (An author-centric observation ;-)
       */
      mouse->options.scaley = mouse->options.scalex * 50 * console.max_x / 80 / console.max_y;
      gpm_report(GPM_PR_DEBUG, GPM_MESS_X_Y_VAL,
                 mouse->options.scalex, mouse->options.scaley);
   }
}

static void handle_repeater(int absolute_dev, Gpm_Event *new_event, Gpm_Event *event)
{
   static struct timeval last;
   struct timeval now;

   if (absolute_dev) {
      /* prepare the values from a absolute device for repeater mode */
      GET_TIME(now);
      if (DIF_TIME(last, now) > 250) {
         event->dx = 0;
         event->dy = 0;
      }
      last = now;

      event->dy = event->dy * ((console.max_x / console.max_y) + 1);
      event->x = new_event->x;
      event->y = new_event->y;
   }
   repeater.type->repeat_fun(event, repeater.fd);
}

static void calculate_clicks(Gpm_Event *event, int click_tmo)
{
   static struct timeval release;
   struct timeval now;

   switch(event->type) {                /* now provide the cooked bits */
      case GPM_DOWN:
         GET_TIME(now);
         if (release.tv_sec && (DIF_TIME(release, now) < click_tmo)) /* check first click */
            event->clicks++, event->clicks %= 3; /* 0, 1 or 2 */
         else
            event->clicks = 0;
         event->type |= GPM_SINGLE << event->clicks;
         break;

      case GPM_UP:
         GET_TIME(release);
         event->type |= GPM_SINGLE << event->clicks;
         break;

      case GPM_DRAG:
         event->type |= GPM_SINGLE << event->clicks;
         break;

      case GPM_MOVE:
         event->clicks = 0;
         break;

      default:
         break;
   }
}

static void snap_to_screen_limits(Gpm_Event *event)
{
   int extent;

   /* The current policy is to force the following behaviour:
    * - At buttons down cursor position must be inside the screen,
    *   though flags are set.
    * - At button up allow going outside by one single step
    *
    * We are using 1-baeed coordinates, like the original "selection".
    * Only one margin will be reported, Y takes priority over X.
    */

   extent = (event->type & (GPM_DRAG|GPM_UP)) ? 1 : 0;

   event->margin = 0;

   if (event->y > console.max_y) {
      event->y = console.max_y + extent;
      extent = 0;
      event->margin = GPM_BOT;
   } else if (event->y <= 0) {
      event->y = 1 - extent;
      extent = 0;
      event->margin = GPM_TOP;
   }

   if (event->x > console.max_x) {
      event->x = console.max_x + extent;
      if (!event->margin) event->margin = GPM_RGT;
   } else if (event->x <= 0) {
      event->x = 1 - extent;
      if (!event->margin) event->margin = GPM_LFT;
   }
}

static int more_data_waiting(int fd)
{
   static struct timeval timeout = {0, 0};
   fd_set fdSet;

   FD_ZERO(&fdSet);
   FD_SET(fd, &fdSet);
   select(fd + 1, &fdSet, NULL_SET, NULL_SET, &timeout/* zero */);

   return FD_ISSET(fd, &fdSet);
}

static int multiplex_buttons(struct micetab *mouse, int new_buttons)
{
   static int left_btn_clicks, mid_btn_clicks, right_btn_clicks;
   int mask;
   int muxed_buttons = 0;

   new_buttons =
      (mouse->options.sequence[new_buttons & 7] & 7) | (new_buttons & ~7);
   mask = new_buttons ^ mouse->buttons;
   mouse->buttons = new_buttons;

   if (mask & GPM_B_LEFT) {
      if (new_buttons & GPM_B_LEFT) left_btn_clicks++;
      else left_btn_clicks--;
   }
   if (left_btn_clicks) muxed_buttons |= GPM_B_LEFT;

   if (mask & GPM_B_MIDDLE) {
      if (new_buttons & GPM_B_MIDDLE) mid_btn_clicks++;
      else mid_btn_clicks--;
   }
   if (mid_btn_clicks) muxed_buttons |= GPM_B_MIDDLE;

   if (mask & GPM_B_RIGHT) {
      if (new_buttons & GPM_B_RIGHT) right_btn_clicks++;
      else right_btn_clicks--;
   }
   if (right_btn_clicks) muxed_buttons |= GPM_B_RIGHT;

   return muxed_buttons;
}

/*-------------------------------------------------------------------
 * call getMouseData to get hardware device data, call mouse device's fun()
 * to retrieve the hardware independent event data, then optionally repeat
 * the data via repeat_fun() to the repeater device
 *-------------------------------------------------------------------*/
static enum mouse_rslt processMouse(struct micetab *mouse, int timeout, int attempt,
                                    Gpm_Event *event, int text_mode)
{
   static int last_active;
   static int fine_dx, fine_dy;
   static int oldB;

   static Gpm_Event nEvent;
   struct Gpm_Type *type = mouse->type;
   struct miceopt *opt = &mouse->options;
   enum mouse_rslt rslt = MOUSE_DATA_OK;
   unsigned char shift_state;
   char *data = NULL;
   int i;

   if (attempt > 1) {          /* continue interrupted cluster loop */
      if (opt->absolute) {
         event->x = nEvent.x;
         event->y = nEvent.y;
      }
      event->dx = nEvent.dx;
      event->dy = nEvent.dy;
      event->buttons = nEvent.buttons;
   } else {
      event->dx = event->dy = 0;
      event->wdx = event->wdy = 0;
      nEvent.modifiers = 0; /* some mice set them */
      i = 0;

      do { /* cluster loop */
         if (!timeout && (data = getMouseData(mouse->dev.fd, type, text_mode)) != NULL) {
            GET_TIME(mouse->timestamp);
         }

         /* in case of timeout data passed to typr->fun() is NULL */
         if ((!timeout && data == NULL) ||
             type->fun(&mouse->dev, &mouse->options, data, &nEvent) == -1) {
            if (!i) return MOUSE_NO_DATA;
            else break;
         }

         event->modifiers = nEvent.modifiers; /* propagate modifiers */

         /* propagate buttons */
         nEvent.buttons = multiplex_buttons(mouse, nEvent.buttons);

         if (!i) event->buttons = nEvent.buttons;

         if (oldB != nEvent.buttons) {
            rslt = MOUSE_MORE_DATA;
            break;
         }

         /* propagate movement */
         if (!opt->absolute) { /* mouse */
            if (abs(nEvent.dx) + abs(nEvent.dy) > opt->delta)
               nEvent.dx *= opt->accel, nEvent.dy *= opt->accel;

            /* increment the reported dx,dy */
            event->dx += nEvent.dx;
            event->dy += nEvent.dy;
         } else { /* a pen */
            /* get dx,dy to check if there has been movement */
            event->dx = nEvent.x - event->x;
            event->dy = nEvent.y - event->y;
         }

         /* propagate wheel */
         event->wdx += nEvent.wdx;
         event->wdy += nEvent.wdy;

      } while (i++ < opt->cluster && more_data_waiting(mouse->dev.fd));
   } /* if(eventFlag) */

   /*....................................... update the button number */

   if ((event->buttons & GPM_B_MIDDLE) && opt->three_button) opt->three_button++;

   /*....................................... we're a repeater, aren't we? */

   if (!text_mode) {
      if (repeater.fd != -1 && !repeater.raw)
         handle_repeater(opt->absolute, &nEvent, event);
      oldB = nEvent.buttons;
      return MOUSE_NO_DATA; /* no events nor information for clients */
   }

/*....................................... no, we arent a repeater, go on */

   /* use fine delta values now, if delta is the information */
   if (!opt->absolute) {
      fine_dx += event->dx;
      fine_dy += event->dy;
      event->dx = fine_dx / opt->scalex;
      event->dy = fine_dy / opt->scaley;
      fine_dx %= opt->scalex;
      fine_dy %= opt->scaley;
   }

   /* up and down, up and down, ... who does a do..while(0) loop ???
      and then makes a break into it... argh ! */

   if (!event->dx && !event->dy && event->buttons == oldB) {
      static time_t awaketime;
      /*
       * Ret information also if never happens, but enough time has elapsed.
       * Note: return 1 will segfault due to missing event->vc; FIXME!
       */
      if (time(NULL) <= awaketime) return MOUSE_NO_DATA;
      awaketime = time(NULL) + 1;
   }

   /*....................................... fill missing fields */
   event->x += event->dx; event->y += event->dy;

   event->vc = get_console_state(&shift_state);
   if (event->vc != last_active) {
      handle_console_resize(event);
      last_active = event->vc;
   }
   event->modifiers |= shift_state;

   if (oldB == event->buttons)
      event->type = (event->buttons ? (GPM_DRAG | GPM_MFLAG) : GPM_MOVE);
   else {
      if (event->buttons > oldB)
         event->type = GPM_DOWN;
      else {
         event->type &= GPM_MFLAG;
         event->type |= GPM_UP;
         event->buttons ^= oldB; /* for button-up, tell which one */
      }
   }
   calculate_clicks(event, opt->time);
   snap_to_screen_limits(event);

   gpm_report(GPM_PR_DEBUG,"M: %3i %3i (%3i %3i) - butt=%i vc=%i cl=%i",
                            event->dx, event->dy, event->x, event->y,
                            event->buttons, event->vc, event->clicks);

   oldB = nEvent.buttons;

   if (opt_special && (event->type & GPM_DOWN) && !processSpecial(event))
      rslt = MOUSE_NO_DATA;

   return rslt;
}

static int wait_for_data(fd_set *connSet, int maxfd, fd_set *selSet)
{
   struct micetab *mouse;
   struct timeval now, timeout = { 0, 0 };
   int mouse_tmo, tmo = INT_MAX;

   GET_TIME(now);

   *selSet = *connSet;
   for (mouse = micelist; mouse; mouse = mouse->next) {
      FD_SET(mouse->dev.fd, selSet);
      maxfd = max(maxfd, mouse->dev.fd);
      if (mouse->dev.timeout >= 0) {
         mouse_tmo = mouse->dev.timeout - DIF_TIME(mouse->timestamp, now);
         tmo = min(tmo, mouse_tmo);
      }
   }

   if (tmo == INT_MAX)
      timeout.tv_sec = SELECT_TIME;
   else if (tmo > 0) {
      timeout.tv_sec = tmo / 1000;
      timeout.tv_usec = (tmo % 1000) * 1000;
   }

   return select(maxfd + 1, selSet, NULL_SET, NULL_SET, &timeout);
}



/*-------------------------------------------------------------------*/
int old_main()
{
   int ctlfd;
   int i, text_mode;
   struct timeval now;
   int maxfd = -1;
   int pending, attempt;
   int timed_out;
   Gpm_Event event;
   struct micetab *mouse;
   struct client_info *ci;
   fd_set selSet, connSet;
   enum mouse_rslt rslt;

   /*....................................... catch interesting signals */
   signal(SIGTERM, gpm_killed);
   signal(SIGINT,  gpm_killed);
   signal(SIGUSR1, gpm_killed); /* usr1 is used by a new gpm killing the old */
   signal(SIGWINCH,gpm_killed); /* winch can be sent if console is resized */
   signal(SIGPIPE, SIG_IGN);    /* WARN */

   init_mice();
   handle_console_resize(&event); /* get screen dimensions */
   ctlfd = listen_for_clients();

   /*....................................... wait for mouse and connections */
   FD_ZERO(&connSet);
   FD_SET(ctlfd, &connSet);
   maxfd = max(maxfd, ctlfd);

   /*--------------------------------------- main loop begins here */

   while (1) {

      pending = wait_for_data(&connSet, maxfd, &selSet);

      if (console_resized) { /* did the console resize? */
         handle_console_resize(&event);
         console_resized = 0;
         signal(SIGWINCH, gpm_killed); /* reinstall handler */
         notify_clients_resize();
      }

      if (pending < 0) {
         if (errno == EBADF) gpm_report(GPM_PR_OOPS, GPM_MESS_SELECT_PROB);
         gpm_report(GPM_PR_ERR, GPM_MESS_SELECT_STRING, strerror(errno));
         continue;
      }

      gpm_report(GPM_PR_DEBUG, GPM_MESS_SELECT_TIMES, pending);

      /*....................................... manage graphic mode */

      /*
       * Be sure to be in text mode. This used to be before select,
       * but actually it only matters if you have events.
       */
      text_mode = is_text_console();
      if (!text_mode && !repeater.type && !repeater.raw) {
         /* if we don't have repeater then there is only one mouse so
          * we can safely use micelist
          */
         close(micelist->dev.fd);
         wait_text_console();
         /* reopen, reinit (the function is only used if we have one mouse device) */
         if ((micelist->dev.fd = open(micelist->device, O_RDWR)) < 0)
            gpm_report(GPM_PR_OOPS, GPM_MESS_OPEN, micelist->device);
         if (micelist->type->init)
            micelist->type->init(&micelist->dev, &micelist->options, micelist->type);
         continue; /* reselect */
      }

      /*....................................... got mouse, process event */
      /*
       * Well, actually, run a loop to maintain inlining of functions without
       * lenghtening the file. This is not too clean a code, but it works....
       */
      GET_TIME(now);
      for (mouse = micelist; mouse; mouse = mouse->next) {
         timed_out = mouse->dev.timeout >= 0 &&
                     DIF_TIME(mouse->timestamp, now) >= mouse->dev.timeout;
         if (timed_out || FD_ISSET(mouse->dev.fd, &selSet)) {
            if (FD_ISSET(mouse->dev.fd, &selSet)) {
               FD_CLR(mouse->dev.fd, &selSet);
               pending--;
            }
            attempt = 0;
            do {
               rslt = processMouse(mouse, timed_out, ++attempt, &event, text_mode);
               if (rslt != MOUSE_NO_DATA) {
                  /* pass it to the client or to the default handler,
                   * or to the selection handler
                   */
                  if (event.vc > MAX_VC) event.vc = 0;
                  if (event.vc == 0 || !cinfo[event.vc] || !do_client(cinfo[event.vc], &event))
                     if (!cinfo[0] || !do_client(cinfo[0], &event))
                        do_selection(&event, mouse->options.three_button);
               }
            } while (rslt == MOUSE_MORE_DATA);
         }
      }

      /*..................... got connection, process it */
      if (pending && FD_ISSET(ctlfd, &selSet)) {
         FD_CLR(ctlfd, &selSet);
         pending--;
         if ((ci = accept_client_connection(ctlfd))) {
            if (ci->data.eventMask & GPM_MOVE) {
               Gpm_Event e = { 0, 0, ci->data.vc, 0, 0,
                               event.x, event.y, GPM_MOVE, 0, 0 };
               do_client(ci, &e);
            }
            FD_SET(ci->fd, &connSet);
            maxfd = max(maxfd, ci->fd);
         }
      }

      /*........................ got request */
      /* itz 10-22-96 check _all_ clients, not just those on top! */
      for (i = 0; pending && i <= MAX_VC; i++) {
         for (ci = cinfo[i]; pending && ci; ci = ci->next) {
            if (FD_ISSET(ci->fd, &selSet)) {
               FD_CLR(ci->fd, &selSet);
               pending--;
               if (!process_client_request(ci, i, event.x, event.y, event.clicks,
                                           event.buttons, micelist->options.three_button)) {
                  FD_CLR(ci->fd, &connSet);
                  remove_client(ci, i);
               }
            }
         }
      }

      /*.................. look for a spare fd */
      /* itz 10-22-96 this shouldn't happen now! */
      for (i = 0; pending && i <= maxfd; i++) {
         if (FD_ISSET(i, &selSet)) {
            FD_CLR(i, &selSet);
            pending--;
            gpm_report(GPM_PR_WARN, GPM_MESS_STRANGE_DATA, i);
         }
      }

      /*................... all done. */
      if (pending) gpm_report(GPM_PR_OOPS, GPM_MESS_SELECT_PROB);
   } /* while(1) */
}
