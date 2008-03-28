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

struct {
   int sample;
   char code[2];
} sampletab[]={
    {  0,"O"},
    { 15,"J"},
    { 27,"K"},
    { 42,"L"},
    { 60,"R"},
    { 85,"M"},
    {125,"Q"},
    {1E9,"N"},
  };


Gpm_Type* I_logi(int fd, unsigned short flags, struct Gpm_Type *type, int argc, char **argv)
{
   int i;
   struct stat buf;
   int busmouse;

   if (check_no_argv(argc, argv)) return NULL;

   /* is this a serial- or a bus- mouse? */
   if(fstat(fd,&buf)==-1) gpm_report(GPM_PR_OOPS,GPM_MESS_FSTAT);
   i=MAJOR(buf.st_rdev);

   /* I don't know why this is herein, but I remove it. I don't think a 
    * hardcoded ttyname in a C file is senseful. I think if the device 
    * exists must be clear before. Not here. */
   /********* if (stat("/dev/ttyS0",&buf)==-1) gpm_oops("stat()"); *****/
   busmouse=(i != MAJOR(buf.st_rdev));

   /* fix the howmany field, so that serial mice have 1, while busmice have 3 */
   type->howmany = busmouse ? 3 : 1;

   /* change from any available speed to the chosen one */
   for (i=9600; i>=1200; i/=2) setspeed(fd, i, (which_mouse->opt_baud), 1 /* write */, flags);

   /* this stuff is peculiar of logitech mice, also for the serial ones */
   write(fd, "S", 1);
   setspeed(fd, (which_mouse->opt_baud), (which_mouse->opt_baud), 1 /* write */,
      CS8 |PARENB |PARODD |CREAD |CLOCAL |HUPCL);

   /* configure the sample rate */
   for (i=0;(which_mouse->opt_sample)<=sampletab[i].sample;i++) ;
   write(fd,sampletab[i].code,1);
   return type;
}

