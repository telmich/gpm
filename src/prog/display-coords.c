/*
 * display-coords.c - a very simple gpm client
 *
 * Copyright 2007-2008 Nico Schottelius <nico-gpm2008 (at) schottelius.org>
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
 *   Compile and link with: gcc -o display-coords -lgpm display-coords.c
 *
 ********/

/*
 * This client is connects to gpm and displays the following values until
 * it is killed by control+c:
 *
 * x/y coords on the screen
 * dx, dy: deltas
 * time: when packet was recievesd
 * dtime: delta to the last packet
 *
 */

#include <unistd.h>           /* write, read, open    */
#include <stdint.h>
#include <stdlib.h>           /* strtol()             */
#include <stdio.h>            /* printf()             */
#include <time.h>             /* time()               */
#include <errno.h>            /* errno                */
#include <sys/select.h>       /* fd_set and FD_*      */
#include "headers/gpm.h"      /* gpm information      */

/* display resulting data */
int display_data(Gpm_Event *event, void *data)
{
   static time_t  last  = 0;
   time_t         now   = time(NULL);
   int            delta;

   delta = now - last;
   last  = now;

   /* display time, delta time */
   printf("[%jd] delta: %ds",(intmax_t)now,delta);
   
   /* display mouse information */
   printf(": x=%2i, y=%2i, dx=%2i, dy=%2i\n", event->x, event->y, event->dx, event->dy);

   return 0;
}

int main(int argc, char **argv)
{
   int            vc;                              /* argv: console number */
   Gpm_Connect    conn;                            /* connection to gpm    */
   fd_set         fds;

   /* select virtual console, 0 if not set */
   vc = (argc == 2) ? strtol(argv[1],NULL,10) : 0;

   conn.eventMask    =  GPM_MOVE; /* read only moves            */
   conn.defaultMask  = ~GPM_HARD; /* inverted GPM_HARD mask    */
   conn.minMod       =  0;
   conn.maxMod       = ~0;

   if(Gpm_Open(&conn,vc) == -1) {
      printf("Cannot connect to gpm!\n");
      return 1;
   }
   if(gpm_fd == -2) {
      printf("I must be run on the console\n");
      return 1;
   }
   

   while(1) { /* read data */
      FD_ZERO(&fds);
      FD_SET(gpm_fd, &fds);

      if (select(gpm_fd+1, &fds, 0, 0, 0) < 0 && errno == EINTR)
         continue;
      if (FD_ISSET(gpm_fd, &fds)) {
         Gpm_Event evt;
         if (Gpm_GetEvent(&evt) > 0) {
            display_data(&evt, NULL);
         } else {
            printf("Gpm_GetEvent failed\n");
         }
      }
   }

   Gpm_Close(); /* close connection */

   return 0;
}
