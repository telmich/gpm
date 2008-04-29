
/*
 * gpm2 - mouse driver for the console
 *
 * Copyright (c) 2007         Nico Schottelius <nico-gpm2 () schottelius.org>
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
 *
 *    handle standard ps2
 ********/

#include <unistd.h>             /* usleep() */
#include <termios.h>            /* tcflush() */
#include <stdio.h>

#include "gpm2-daemon.h"
#include "gpm2-generic.h"

/* protocol handler stolen from gpm1/mice.c */

/* ps2 protocol description
 *
 * 3 Bytes per packet
 *
 * first bit of data[0]:   Left Button
 * second bit of data[0]:  Right Button
 * third bit of data[0]:   Middle Button
 * fourth bit of data[0]:  Always 1
 * fifth bit of data[0]:   X Sign
 * sixth bit of data[0]:   Y Sign
 * seventh bit of data[0]: X Overflow
 * eighth bit of data[0]:  Y Overflow
 *
 * data[1]: X Movement
 * data[2]: Y Movement
 */

/* standard ps2 */

/* FIXME: implement error handling and options as described on
 * http://www.computer-engineering.org/ps2mouse/
 */
int gpm2_open_ps2(int fd)
{
   // static unsigned char s[] = { 246, 230, 244, 243, 100, 232, 3, };
//   write(fd, s, sizeof (s));

//   static unsigned char s[] = { 246, 230, 244, 243, 100, 232, 3, };

   char obuf, ibuf;

   /*
    * FIXME: check return 
    */

   obuf = 0xff;                 /* reset mouse */
   write(fd, &obuf, 1);
   read(fd, &ibuf, 1);
   printf("RX: %#hhx\n", ibuf);
//   if(ibuf != 0xfa) return 0;

//   obuf = 0xf6; /* set mouse to default */
//   write(fd, &obuf, 1);
//   read(fd,&ibuf,1);
//   printf("DF: %#hhx\n",ibuf);

   obuf = 0xf2;                 /* device id */
   write(fd, &obuf, 1);
   read(fd, &ibuf, 1);
   printf("ST: %#hhx\n", ibuf);
   read(fd, &ibuf, 1);
   printf("DI: %#hhx\n", ibuf);

   /*
    * set resolution: 0xe8 
    */

   // if(scaling = 2) { e7h
   // 
   // 0xe6 scaling =1

   /*
    * do 0xf2, get device id and print it out 
    */
   /*
    * do 0xe9 and print out values as status 
    */

   /*
    * FIXME: time correct? 
    */
   usleep(30000);
   tcflush(fd, TCIFLUSH);
   return 0;

}

/* returs gpm2-readable data */
//int *gpm2_decode_ps2(char *data, struct mousedata *decoded)
int gpm2_decode_ps2(int fd)
{
   char data[3];

   data[0] = 0;
   data[1] = 0;
   data[2] = 0;

   read(fd, data, 3);

   if(data[0] & 0x01)
      mini_printf("left!\n", 1);
   if(data[0] & 0x02)
      mini_printf("right!\n", 1);
   if(data[0] & 0x04)
      mini_printf("middle!\n", 1);

   /*
    * decoded->button_left = data[0] & 0x01; decoded->button_right = data[0] &
    * 0x02; decoded->button_middle = data[0] & 0x04;
    * 
    * decoded-> 
    */

   return 1;
}
