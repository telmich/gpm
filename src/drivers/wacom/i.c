
/*
 * general purpose mouse (gpm)
 *
 * Copyright (c) 2008        Nico Schottelius <nico-gpm2008 at schottelius.org>
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
 *
 ********/

#include <termios.h>            /* termios */
#include <unistd.h>             /* usleep, write */
#include <string.h>
#include <sys/select.h>         /* select */

#include "mice.h"               /* check_no_argv */
#include "types.h"              /* Gpm_type */
#include "message.h"            /* gpm_report */
#include "wacom.h"              /* wacom */

Gpm_Type *I_wacom(int fd, unsigned short flags, struct Gpm_Type *type, int argc,
                  char **argv)
{

/* wacom graphire tablet */
#define UD_RESETBAUD     "\r$"  /* reset baud rate to default (wacom V) */
   /*
    * or switch to wacom IIs (wacomIV) 
    */
#define UD_RESET         "#\r"  /* Reset tablet and enable WACOM IV */
#define UD_SENDCOORDS    "ST\r" /* Start sending coordinates */
#define UD_FIRMID        "~#\r" /* Request firmware ID string */
#define UD_COORD         "~C\r" /* Request max coordinates */
#define UD_STOP          "\nSP\r"       /* stop sending coordinates */

   flags = 0;                   /* FIXME: 1.99.13 */

   void reset_wacom() {
      /*
       * Init Wacom communication; this is modified from xf86Wacom.so module 
       */
      /*
       * Set speed to 19200 
       */
      setspeed(fd, 1200, 19200, 0, B19200 | CS8 | CREAD | CLOCAL | HUPCL);
      /*
       * Send Reset Baudrate Command 
       */
      write(fd, UD_RESETBAUD, strlen(UD_RESETBAUD));
      usleep(250000);
      /*
       * Send Reset Command 
       */
      write(fd, UD_RESET, strlen(UD_RESET));
      usleep(75000);
      /*
       * Set speed to 9600bps 
       */
      setspeed(fd, 1200, 9600, 0, B9600 | CS8 | CREAD | CLOCAL | HUPCL);
      /*
       * Send Reset Command 
       */
      write(fd, UD_RESET, strlen(UD_RESET));
      usleep(250000);
      write(fd, UD_STOP, strlen(UD_STOP));
      usleep(100000);
   }

   int wait_wacom() {
      /*
       *  Wait up to 200 ms for Data from Tablet.
       *  Do not read that data.
       *  Give back 0 on timeout condition, -1 on error and 1 for DataPresent
       */
      struct timeval timeout;

      fd_set readfds;

      int err;

      FD_ZERO(&readfds);
      FD_SET(fd, &readfds);
      timeout.tv_sec = 0;
      timeout.tv_usec = 200000;
      err = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);
      return ((err > 0) ? 1 : err);
   }

   char buffer[50], *p;

   int RequestData(char *cmd) {
      int err;

      /*
       * Send cmd if not null, and get back answer from tablet.
       * Get Data to buffer until full or timeout.
       * Give back 0 for timeout and !0 for buffer full
       */
      if(cmd)
         write(fd, cmd, strlen(cmd));
      memset(buffer, 0, sizeof(buffer));
      p = buffer;
      err = wait_wacom();
      while(err != -1 && err && (p - buffer) < (int) (sizeof(buffer) - 1)) {
         p += read(fd, p, (sizeof(buffer) - 1) - (p - buffer));
         err = wait_wacom();
      }
      /*
       * return 1 for buffer full 
       */
      return ((strlen(buffer) >= (sizeof(buffer) - 1)) ? !0 : 0);
   }

   /*
    * We do both modes, relative and absolute, with the same function.
    * If WacomAbsoluteWanted is !0 then that function calculates
    * absolute values, else relative ones.
    * We set this by the -o switch:
    * gpm -t wacom -o absolute   # does absolute mode
    * gpm -t wacom               # does relative mode
    * gpm -t wacom -o relative   # does relative mode
    */

   /*
    * accept boolean options absolute and relative 
    */
   static argv_helper optioninfo[] = {
    {"absolute", ARGV_BOOL, u: {iptr: &WacomAbsoluteWanted}, value:!0},
    {"relative", ARGV_BOOL, u: {iptr: &WacomAbsoluteWanted}, value:0},
    {"", ARGV_END, u: {iptr: &WacomAbsoluteWanted}, value:0}
   };
   parse_argv(optioninfo, argc, argv);
   type->absolute = WacomAbsoluteWanted;
   reset_wacom();

   /*
    * "Flush" input queque 
    */
   while(RequestData(NULL)) ;

   /*
    * read WACOM-ID 
    */
   RequestData(UD_FIRMID);

   /*
    * Search for matching modell 
    */
   for(WacomModell = 0;
       WacomModell < (int) (sizeof(wcmodell) / sizeof(struct WC_MODELL));
       WacomModell++) {
      if(!strncmp(buffer + 2, wcmodell[WacomModell].magic, 2)) {
         /*
          * Magic matches, modell found 
          */
         wmaxx = wcmodell[WacomModell].maxX;
         wmaxy = wcmodell[WacomModell].maxY;
         break;
      }
   }
   if(WacomModell >= (int) (sizeof(wcmodell) / sizeof(struct WC_MODELL)))
      WacomModell = -1;
   gpm_report(GPM_PR_INFO, GPM_MESS_WACOM_MOD, type->absolute ? 'A' : 'R',
              (WacomModell == (-1)) ? "Unknown" : wcmodell[WacomModell].name,
              buffer + 2);

   /*
    * read Wacom max size 
    */
   if(WacomModell != (-1) && (!wcmodell[WacomModell].maxX)) {
      RequestData(UD_COORD);
      sscanf(buffer + 2, "%d,%d", &wmaxx, &wmaxy);
      wmaxx = (wmaxx - wcmodell[WacomModell].border);
      wmaxy = (wmaxy - wcmodell[WacomModell].border);
   }
   write(fd, UD_SENDCOORDS, 4);

   return type;
}
