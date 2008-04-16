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

#include <unistd.h>                 /* usleep, write     */
#include <sys/select.h>             /* select            */
#include <termios.h>                /* termios           */
#include <string.h>                 /* string            */

#include "types.h"                  /* Gpm_type         */
#include "mice.h"                   /* setspeed         */

extern int  SUMMA_BORDER;
extern int  summamaxx;
extern int  summamaxy;
extern signed char summaid;


Gpm_Type *I_summa(int fd, unsigned short flags, struct Gpm_Type *type, int argc, char **argv)
{

   flags = argc = 0; /* FIXME: 1.99.13 */
   argv = NULL;

   void resetsumma()
   {
      write(fd,0,1); /* Reset */
      usleep(400000); /* wait */
   }
   int waitsumma()
   {
      struct timeval timeout;
      fd_set readfds;
      int err;
      FD_ZERO(&readfds);  FD_SET(fd, &readfds);
      timeout.tv_sec = 0; timeout.tv_usec = 200000;
      err = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);
      return(err);
   }
   int err;
   char buffer[255];
   char config[5];
   /* Summasketchtablet (from xf86summa.o: thanks to Steven Lang <tiger@tyger.org>)*/
   #define SS_PROMPT_MODE  "B"     /* Prompt mode */
   #define SS_FIRMID       "z?"    /* Request firmware ID string */
   #define SS_500LPI       "h"     /* 500 lines per inch */
   #define SS_READCONFIG   "a"
   #define SS_ABSOLUTE     "F"     /* Absolute Mode */
   #define SS_TABID0       "0"     /* Tablet ID 0 */
   #define SS_UPPER_ORIGIN "b"     /* Origin upper left */
   #define SS_BINARY_FMT   "zb"    /* Binary reporting */
   #define SS_STREAM_MODE  "@"     /* Stream mode */
   /* Geniustablet (hisketch.txt)*/
   #define GEN_MMSERIES ":"
   char GEN_MODELL=0x7f;

   /* Set speed to 9600bps */
   setspeed (fd, 1200, 9600, 1, B9600|CS8|CREAD|CLOCAL|HUPCL|PARENB|PARODD);
   resetsumma();

   write(fd, SS_PROMPT_MODE, strlen(SS_PROMPT_MODE));

   if (strstr(type->name,"acecad")!=NULL) summaid=11;

   if (summaid<0) { /* Summagraphics test */
      /* read the Summa Firm-ID */
      write(fd, SS_FIRMID, strlen(SS_FIRMID));
      err=waitsumma();
      if (!((err == -1) || (!err))) {
         summaid=10; /* Original Summagraphics */
         read(fd, buffer, 255); /* Read Firm-ID */
      }
   }

   if (summaid<0) { /* Genius-test */
      resetsumma();
      write(fd,GEN_MMSERIES,1);
      write(fd,&GEN_MODELL,1); /* Read modell */
      err=waitsumma();
      if (!((err == -1) || (!err))) { /* read Genius-ID */
           err=waitsumma();
         if (!((err == -1) || (!err))) {
            err=waitsumma();
            if (!((err == -1) || (!err))) {
               read(fd,&config,1);
               summaid=(config[0] & 224) >> 5; /* genius tablet-id (0-7)*/
            }
         }
      }
   } /* end of Geniustablet-test */

   /* unknown tablet ?*/
   if ((summaid<0) || (summaid==11)) {
      resetsumma();
      write(fd, SS_BINARY_FMT SS_PROMPT_MODE, 3);
   }

   /* read tablet size */
   err=waitsumma();
   if (!((err == -1) || (!err))) read(fd,buffer,sizeof(buffer));
   write(fd,SS_READCONFIG,1);
   read(fd,&config,5);
   summamaxx=(config[2]<<7 | config[1])-(SUMMA_BORDER*2);
   summamaxy=(config[4]<<7 | config[3])-(SUMMA_BORDER*2);

   write(fd,SS_ABSOLUTE SS_STREAM_MODE SS_UPPER_ORIGIN,3);
   if (summaid<0) write(fd,SS_500LPI SS_TABID0 SS_BINARY_FMT,4);

   return type;
}
