/*
 * console.c - GPM console and selection/paste handling
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
#include <errno.h>
#include <unistd.h>        /* select(); */
#include <time.h>          /* time() */
#include <sys/fcntl.h>     /* O_RDONLY */
#include <sys/stat.h>      /* mkdir()  */
#include <asm/types.h>     /* __u32 */

#include <linux/vt.h>      /* VT_GETSTATE */
#include <sys/kd.h>        /* KDGETMODE */
#include <termios.h>       /* winsize */

#include "headers/gpmInt.h"
#include "headers/console.h"
#include "headers/message.h"

#ifndef HAVE___U32
# ifndef _I386_TYPES_H /* /usr/include/asm/types.h */
typedef unsigned int __u32;
# endif
#endif

struct gpm_console console = { 0, DEF_LUT, 0, 0 };

/*-------------------------------------------------------------------*/
static int count_digits(int num)
{
   int digits = 1;

   while ((num /= 10))
      digits++;

   return digits;
}

/*-------------------------------------------------------------------*/
char *compose_vc_name(int vc)
{
   char  *tty;

   tty = malloc(strlen(console.device) + count_digits(vc) + sizeof(char));
   if (tty) {
      /* console is /dev/vc/0 or /dev/tty0 and we trimming the ending 0 */
      strncpy(tty, console.device, strlen(console.device) - 1);
      sprintf(&tty[strlen(console.device) - 1], "%d", vc);
   }

   return tty;
}

/*-------------------------------------------------------------------*/
int open_console(int mode)
{
   int fd;

   if ((fd = open(console.device, mode)) < 0)
      gpm_report(GPM_PR_OOPS, GPM_MESS_OPEN_CON);

   return fd;
}

/*-------------------------------------------------------------------*/
int is_text_console(void)
{
   int fd;
   int kd_mode;

   fd = open_console(O_RDONLY);
   if (ioctl(fd, KDGETMODE, &kd_mode) < 0)
      gpm_report(GPM_PR_OOPS, GPM_MESS_IOCTL_KDGETMODE);
   close(fd);

   return kd_mode == KD_TEXT;
}

/*-------------------------------------------------------------------*/
void wait_text_console(void)
{
   do {
      sleep(2);
   } while (!is_text_console());
}

/*-------------------------------------------------------------------*/
void refresh_console_size(void)
{
   struct winsize win;
   int fd = open_console(O_RDONLY);

   ioctl(fd, TIOCGWINSZ, &win);
   close(fd);

   if (!win.ws_col || !win.ws_row) {
      gpm_report(GPM_PR_DEBUG, GPM_MESS_ZERO_SCREEN_DIM);
      console.max_x = 80; console.max_y = 25;
   } else {
      console.max_x = win.ws_col; console.max_y = win.ws_row;
   }
   gpm_report(GPM_PR_DEBUG, GPM_MESS_SCREEN_SIZE, console.max_x, console.max_y);
}

/*-------------------------------------------------------------------*/
int get_console_state(unsigned char *shift_state)
{
   struct vt_stat stat;
   int fd;

   fd = open_console(O_RDONLY);

   *shift_state = 6; /* code for the ioctl */
   if (ioctl(fd, TIOCLINUX, shift_state) < 0)
      gpm_report(GPM_PR_OOPS, GPM_MESS_GET_SHIFT_STATE);

   if (ioctl(fd, VT_GETSTATE, &stat) < 0)
      gpm_report(GPM_PR_OOPS, GPM_MESS_GET_CONSOLE_STAT);

   close(fd);

   return stat.v_active;
}

/*-------------------------------------------------------------------*/
int is_console_owner(int vc, uid_t uid)
{
   struct stat statbuf;
   char  *tty;
   int   rc;

   if ((tty = compose_vc_name(vc)) == NULL)
      gpm_report(GPM_PR_OOPS, GPM_MESS_NO_MEM);

   if ((rc = stat(tty, &statbuf)) == -1)
      gpm_report(GPM_PR_ERR, GPM_MESS_STAT_FAILS, tty);

   free(tty);

   return rc != -1 && uid == statbuf.st_uid;
}

/*-------------------------------------------------------------------*/
/* octal digit */
static int isodigit(const unsigned char c)
{
   return ((c & ~7) == '0');
}

/*-------------------------------------------------------------------*/
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

/*-------------------------------------------------------------------*/
/* description missing! FIXME */
void console_load_lut(void)
{
   extern int errno;
   int i, c, fd;
   unsigned char this, next;
   static __u32 long_array[9] = {
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

   for (i = 0; console.charset[i]; ) {
      i += getsym(console.charset + i, &this);
      if (console.charset[i] == '-' && console.charset[i + 1] != '\0')
         i += getsym(console.charset + i + 1, &next) + 1;
      else
         next = this;
      for (c = this; c <= next; c++)
         inwordLut[c >> 5] |= 1 << (c & 0x1F);
   }

   fd = open_console(O_WRONLY);

   if (ioctl(fd, TIOCLINUX, &long_array) < 0) { /* fd <0 is checked */
      if (errno == EPERM && getuid())
         gpm_report(GPM_PR_WARN, GPM_MESS_ROOT); /* why do we still continue?*/
      else if (errno == EINVAL)
         gpm_report(GPM_PR_OOPS, GPM_MESS_CSELECT);
   }
   close(fd);
}

/*-------------------------------------------------------------------*/
/* Returns the name of the console (/dev/tty0 or /dev/vc/0)          */
/* Also fills console.device                                         */
char *get_console_name()
{
   struct stat buf;

   /* first try the devfs device, because in the next time this will be
    * the preferred one. If that fails, take the old console */

   /* Check for open new console */
   if (stat(GPM_DEVFS_CONSOLE, &buf) == 0)
      console.device = GPM_DEVFS_CONSOLE;

   /* Failed, try OLD console */
   else if (stat(GPM_OLD_CONSOLE, &buf) == 0)
      console.device = GPM_OLD_CONSOLE;
   else
      gpm_report(GPM_PR_OOPS, "Can't determine console device");

   return console.device;
}

