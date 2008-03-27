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

#include "types.h"                  /* Gpm_type         */


Gpm_Type* I_serial(int fd, unsigned short flags,
    struct Gpm_Type *type, int argc, char **argv)
{
   int i; unsigned char c;
   fd_set set; struct timeval timeout={0,0}; /* used when not debugging */

   /* accept "-o dtr", "-o rts" and "-o both" */
   if (option_modem_lines(fd, argc, argv)) return NULL;

#ifndef DEBUG
   /* flush any pending input (thanks, Miguel) */
   FD_ZERO(&set);
   for(i=0; /* always */ ; i++) {
      FD_SET(fd,&set);
      switch(select(fd+1,&set,(fd_set *)NULL,(fd_set *)NULL,&timeout/*zero*/)){
         case  1: if (read(fd,&c,1)==0) break;
         case -1: continue;
      }
      break;
   }

   if (type->fun==M_logimsc) write(fd, "QU", 2 );

#if 0 /* Did this ever work? -- I don't know, but should we not remove it,
       * if it doesn't work ??? -- Nico */
   if (type->fun==M_ms && i==2 && c==0x33)  { /* Aha.. a mouseman... */
      gpm_report(GPM_PR_INFO,GPM_MESS_MMAN_DETECTED);
      return mice; /* it is the first */
   }
#endif
#endif

   /* Non mman: change from any available speed to the chosen one */
   for (i=9600; i>=1200; i/=2)
      setspeed(fd, i, (which_mouse->opt_baud), (type->fun != M_mman) /* write */, flags);

   /*
    * reset the MouseMan/TrackMan to use the 3/4 byte protocol
    * (Stephen Lee, sl14@crux1.cit.cornell.edu)
    * Changed after 1.14; why not having "I_mman" now?
    */
   if (type->fun==M_mman) {
      setspeed(fd, 1200, 1200, 0, flags); /* no write */
      write(fd, "*X", 2);
      setspeed(fd, 1200, (which_mouse->opt_baud), 0, flags); /* no write */
      return type;
   }

   if(type->fun==M_geni) {
      gpm_report(GPM_PR_INFO,GPM_MESS_INIT_GENI);
      setspeed(fd, 1200, 9600, 1, flags); /* write */
      write(fd, ":" ,1);
      write(fd, "E" ,1); /* setup tablet. relative mode, resolution... */
      write(fd, "@" ,1); /* setup tablet. relative mode, resolution... */
    }

   if (type->fun==M_synaptics_serial) {
      int packet_length;

      setspeed (fd, 1200, 1200, 1, flags);
      packet_length = syn_serial_init (fd);
      setspeed (fd, 1200, 9600, 1, flags);

      type->packetlen = packet_length;
      type->howmany   = packet_length;
    }

   if (type->fun==M_vsxxx_aa) {
      setspeed (fd, 4800, 4800, 0, flags); /* no write */
      write(fd, "R", 1); /* initialize a mouse; without getting an "R" */
                         /* a mouse does not send a bytestream         */
   }

   return type;
}

