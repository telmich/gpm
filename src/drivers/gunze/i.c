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

#include <termios.h>                /* termios           */
#include "types.h"                  /* Gpm_type          */


/* simple initialization for the gunze touchscreen */
Gpm_Type *I_gunze(int fd, unsigned short flags, struct Gpm_Type *type, int argc, char **argv)
{
   struct termios tty;
   FILE *f;
   char s[80];
   int i, calibok = 0;

   #define GUNZE_CALIBRATION_FILE SYSCONFDIR "/gpm-calibration"
   /* accept a few options */
   static argv_helper optioninfo[] = {
      {"smooth",   ARGV_INT, u: {iptr: &gunze_avg}},
      {"debounce", ARGV_INT, u: {iptr: &gunze_debounce}},
      /* FIXME: add corner tapping */
      {"",       ARGV_END}
   };
   parse_argv(optioninfo, argc, argv);

   /* check that the baud rate is valid */
   if ((which_mouse->opt_baud) == DEF_BAUD) (which_mouse->opt_baud) = 19200; /* force 19200 as default */
   if ((which_mouse->opt_baud) != 9600 && (which_mouse->opt_baud) != 19200) {
      gpm_report(GPM_PR_ERR,GPM_MESS_GUNZE_WRONG_BAUD,option.progname, argv[0]);
      (which_mouse->opt_baud) = 19200;
   }
   tcgetattr(fd, &tty);
   tty.c_iflag = IGNBRK | IGNPAR;
   tty.c_oflag = 0;
   tty.c_lflag = 0;
   tty.c_line = 0;
   tty.c_cc[VTIME] = 0;
   tty.c_cc[VMIN] = 1;
   tty.c_cflag = ((which_mouse->opt_baud) == 9600 ? B9600 : B19200) |CS8|CREAD|CLOCAL|HUPCL;
   tcsetattr(fd, TCSAFLUSH, &tty);

   /* FIXME: try to find some information about the device */

   /* retrieve calibration, if not existent, use defaults (uncalib) */
   f = fopen(GUNZE_CALIBRATION_FILE, "r");
   if (f) {
      fgets(s, 80, f); /* discard the comment */
      if (fscanf(f, "%d %d %d %d", gunze_calib, gunze_calib+1,
         gunze_calib+2, gunze_calib+3) == 4)
         calibok = 1;
   /* Hmm... check */
      for (i=0; i<4; i++)
         if (gunze_calib[i] & ~1023) calibok = 0;
      if (gunze_calib[0] == gunze_calib[2]) calibok = 0;
      if (gunze_calib[1] == gunze_calib[3]) calibok = 0;
      fclose(f);
   }
   if (!calibok) {
      gpm_report(GPM_PR_ERR,GPM_MESS_GUNZE_CALIBRATE, option.progname);
      gunze_calib[0] = gunze_calib[1] = 128; /* 1/8 */
      gunze_calib[2] = gunze_calib[3] = 896; /* 7/8 */
   }
   return type;
}

