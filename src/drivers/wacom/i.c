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


/*
 * mice.c - mouse definitions for gpm-Linux
 *
 * Copyright (C) 1993        Andrew Haylett <ajh@gec-mrc.co.uk>
 * Copyright (C) 1994-2000   Alessandro Rubini <rubini@linux.it>
 * Copyright (C) 1998,1999   Ian Zimmerman <itz@rahul.net>
 * Copyright (C) 2001-2008   Nico Schottelius <nico-gpm2008 at schottelius.org>
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

/*
 * This file is part of the mouse server. The information herein
 * is kept aside from the rest of the server to ease fixing mouse-type
 * issues. Each mouse type is expected to fill the `buttons', `dx' and `dy'
 * fields of the Gpm_Event structure and nothing more.
 *
 * Absolute-pointing devices (support by Marc Meis), are expecting to
 * fit `x' and `y' as well. Unfortunately, to do it the window size must
 * be accessed. The global variable "win" is available for that use.
 *
 * The `data' parameter points to a byte-array with event data, as read
 * by the mouse device. The mouse device should return a fixed number of
 * bytes to signal an event, and that exact number is read by the server
 * before calling one of these functions.
 *
 * The conversion function defined here should return 0 on success and -1
 * on failure.
 *
 * Refer to the definition of Gpm_Type to probe further.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h> /* stat() */
#include <sys/time.h> /* select() */

#include <linux/kdev_t.h> /* MAJOR */
#include <linux/keyboard.h>

/* JOYSTICK */
#ifdef HAVE_LINUX_JOYSTICK_H
#include <linux/joystick.h>
#endif

/* EV DEVICE */
#ifdef HAVE_LINUX_INPUT_H
#include <linux/input.h>
#endif /* HAVE_LINUX_INPUT_H */


#include "headers/daemon.h"

#include "headers/gpmInt.h"
#include "headers/twiddler.h"
#include "headers/synaptics.h"
#include "headers/message.h"

/*========================================================================*/
/*  real absolute coordinates for absolute devices,  not very clean */
/*========================================================================*/

#define REALPOS_MAX 16383 /*  min 0 max=16383, but due to change.  */
int realposx=-1, realposy=-1;

#define GPM_B_BOTH (GPM_B_LEFT|GPM_B_RIGHT)

/*========================================================================*/
/* Then, mice should be initialized */

struct {
   int sample; char code[2];
} sampletab[]={
    {  0,"O"},
    { 15,"J"},
    { 27,"K"},
    { 42,"L"},
    { 60,"R"},
    { 85,"M"},
    {125,"Q"},
    {1E9,"N"}, };

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

Gpm_Type* I_logi(int fd, unsigned short flags,
       struct Gpm_Type *type, int argc, char **argv)
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

Gpm_Type *I_wacom(int fd, unsigned short flags,
                         struct Gpm_Type *type, int argc, char **argv)
{
/* wacom graphire tablet */
#define UD_RESETBAUD     "\r$"      /* reset baud rate to default (wacom V) */
                                    /* or switch to wacom IIs (wacomIV)     */ 
#define UD_RESET         "#\r"      /* Reset tablet and enable WACOM IV     */
#define UD_SENDCOORDS    "ST\r"     /* Start sending coordinates            */
#define UD_FIRMID        "~#\r"     /* Request firmware ID string           */
#define UD_COORD         "~C\r"     /* Request max coordinates              */
#define UD_STOP          "\nSP\r"   /* stop sending coordinates             */

   void reset_wacom()
   { 
      /* Init Wacom communication; this is modified from xf86Wacom.so module */
      /* Set speed to 19200 */
      setspeed (fd, 1200, 19200, 0, B19200|CS8|CREAD|CLOCAL|HUPCL);
      /* Send Reset Baudrate Command */ 
      write(fd, UD_RESETBAUD, strlen(UD_RESETBAUD));
      usleep(250000);   
      /* Send Reset Command */
      write(fd, UD_RESET,     strlen(UD_RESET));
      usleep(75000);
      /* Set speed to 9600bps */
      setspeed (fd, 1200, 9600, 0, B9600|CS8|CREAD|CLOCAL|HUPCL);
      /* Send Reset Command */
      write(fd, UD_RESET, strlen(UD_RESET));
      usleep(250000);  
      write(fd, UD_STOP, strlen(UD_STOP));
      usleep(100000);
   }

   int wait_wacom()
   {
      /* 
       *  Wait up to 200 ms for Data from Tablet. 
       *  Do not read that data.
       *  Give back 0 on timeout condition, -1 on error and 1 for DataPresent
       */
      struct timeval timeout;
      fd_set readfds;
      int err;
      FD_ZERO(&readfds);  FD_SET(fd, &readfds);
      timeout.tv_sec = 0; timeout.tv_usec = 200000;
      err = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);
      return((err>0)?1:err);
   }

   char buffer[50], *p;

   int RequestData(char *cmd)
   {
      int err;
      /* 
       * Send cmd if not null, and get back answer from tablet. 
       * Get Data to buffer until full or timeout.
       * Give back 0 for timeout and !0 for buffer full
       */
      if (cmd) write(fd,cmd,strlen(cmd));
      memset(buffer,0,sizeof(buffer)); p=buffer;
      err=wait_wacom();
      while (err != -1 && err && (p-buffer)<(sizeof(buffer)-1)) {
         p+= read(fd,p,(sizeof(buffer)-1)-(p-buffer));
         err=wait_wacom();
      }
      /* return 1 for buffer full */
      return ((strlen(buffer) >= (sizeof(buffer)-1))? !0 :0);
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

   /* accept boolean options absolute and relative */
   static argv_helper optioninfo[] = {
         {"absolute",  ARGV_BOOL, u: {iptr: &WacomAbsoluteWanted}, value: !0},
         {"relative",  ARGV_BOOL, u: {iptr: &WacomAbsoluteWanted}, value:  0},
         {"",       ARGV_END} 
   };
   parse_argv(optioninfo, argc, argv); 
   type->absolute = WacomAbsoluteWanted;
   reset_wacom();

   /* "Flush" input queque */
   while(RequestData(NULL)) ;

   /* read WACOM-ID */
   RequestData(UD_FIRMID); 

   /* Search for matching modell */
   for(WacomModell=0;
      WacomModell< (sizeof(wcmodell) / sizeof(struct WC_MODELL));
      WacomModell++ ) {
      if (!strncmp(buffer+2,wcmodell[WacomModell].magic, 2)) {
         /* Magic matches, modell found */
         wmaxx=wcmodell[WacomModell].maxX;
         wmaxy=wcmodell[WacomModell].maxY;
         break;
      }
   }
   if(WacomModell >= (sizeof(wcmodell) / sizeof(struct WC_MODELL))) 
      WacomModell=-1;
   gpm_report(GPM_PR_INFO,GPM_MESS_WACOM_MOD, type->absolute? 'A':'R',
                (WacomModell==(-1))? "Unknown" : wcmodell[WacomModell].name,
                buffer+2);
   
   /* read Wacom max size */
   if(WacomModell!=(-1) && (!wcmodell[WacomModell].maxX)) {
      RequestData(UD_COORD);
      sscanf(buffer+2, "%d,%d", &wmaxx, &wmaxy);
      wmaxx = (wmaxx-wcmodell[WacomModell].border);
      wmaxy = (wmaxy-wcmodell[WacomModell].border);
   }
   write(fd,UD_SENDCOORDS,4);

   return type;
}

Gpm_Type *I_pnp(int fd, unsigned short flags,
             struct Gpm_Type *type, int argc, char **argv)
{  
   struct termios tty;

   /* accept "-o dtr", "-o rts" and "-o both" */
   if (option_modem_lines(fd, argc, argv)) return NULL;

   /*
    * Just put the device to 1200 baud. Thanks to Francois Chastrette
    * for his great help and debugging with his own pnp device.
    */
   tcgetattr(fd, &tty);
    
   tty.c_iflag = IGNBRK | IGNPAR;
   tty.c_oflag = 0;
   tty.c_lflag = 0;
   tty.c_line = 0;
   tty.c_cc[VTIME] = 0;
   tty.c_cc[VMIN] = 1;
   tty.c_cflag = flags | B1200;
   tcsetattr(fd, TCSAFLUSH, &tty); /* set parameters */

   /*
    * Don't read the silly initialization string. I don't want to see
    * the vendor name: it is only propaganda, with no information.
    */
   
   return type;
}

/*
 * Sends the SEND_ID command to the ps2-type mouse.
 * Return one of GPM_AUX_ID_...
 */
int read_mouse_id(int fd)
{
   unsigned char c = GPM_AUX_SEND_ID;
   unsigned char id;

   write(fd, &c, 1);
   read(fd, &c, 1);
   if (c != GPM_AUX_ACK) {
      return(GPM_AUX_ID_ERROR);
   }
   read(fd, &id, 1);

   return(id);
}


/*========================================================================*/
/* Finally, the table */
#define STD_FLG (CREAD|CLOCAL|HUPCL)

 /*
   * For those who are trying to add a new type, here a brief
   * description of the structure. Please refer to gpmInt.h and gpm.c
   * for more information:
   *
   * The first three strings are the name, an help line, a long name (if any)
   * Then come the functions: the decoder and the initializazion function
   *     (called I_* and M_*)
   * Follows an array of four bytes: it is the protocol-identification, based
   *     on the first two bytes of a packet: if
   *     "((byte0 & proto[0]) == proto[1]) && ((byte1 & proto[2]) == proto[3])"
   *     then we are at the beginning of a packet.
   * The following numbers are:
   *     bytes-per-packet,
   *     bytes-to-read (use 1, bus mice are pathological)
   *     has-extra-byte (boolean, for mman pathological protocol)
   *     is-absolute-coordinates (boolean)
   * Finally, a pointer to a repeater function, if any.
   */

Gpm_Type mice[]={

   {"mman", "The \"MouseMan\" and similar devices (3/4 bytes per packet).",
           "Mouseman", M_mman, I_serial, CS7 | STD_FLG, /* first */
                                {0x40, 0x40, 0x40, 0x00}, 3, 1, 1, 0, 0},
   {"ms",   "The original ms protocol, with a middle-button extension.",
           "", M_ms, I_serial, CS7 | STD_FLG,
                                {0x40, 0x40, 0x40, 0x00}, 3, 1, 0, 0, 0},
   {"acecad",  "Acecad tablet absolute mode(Sumagrapics MM-Series mode)",
           "", M_summa, I_summa, STD_FLG,
                                {0x80, 0x80, 0x00, 0x00}, 7, 1, 0, 1, 0}, 
   {"bare", "Unadorned ms protocol. Needed with some 2-buttons mice.",
           "Microsoft", M_bare, I_serial, CS7 | STD_FLG,
                                {0x40, 0x40, 0x40, 0x00}, 3, 1, 0, 0, 0},
   {"bm",   "Micro$oft busmice and compatible devices.",
           "BusMouse", M_bm, I_empty, STD_FLG, /* bm is sun */
                                {0xf8, 0x80, 0x00, 0x00}, 3, 3, 0, 0, 0},
   {"brw",  "Fellowes Browser - 4 buttons (and a wheel) (dual protocol?)",
           "", M_brw, I_pnp, CS7 | STD_FLG,
                                {0xc0, 0x40, 0xc0, 0x00}, 4, 1, 0, 0, 0},
   {"cal", "Calcomp UltraSlate",
           "", M_calus, I_calus, CS8 | CSTOPB | STD_FLG,
                                {0x80, 0x80, 0x80, 0x00}, 6, 6, 0, 1, 0},
   {"calr", "Calcomp UltraSlate - relative mode",
           "", M_calus_rel, I_calus, CS8 | CSTOPB | STD_FLG,
                                {0x80, 0x80, 0x80, 0x00}, 6, 6, 0, 0, 0},
  {"etouch",  "EloTouch touch-screens (only button-1 events, by now)",
           "", M_etouch, I_etouch, STD_FLG,
                                {0xFF, 0x55, 0xFF, 0x54}, 7, 1, 0, 1, NULL}, 
#ifdef HAVE_LINUX_INPUT_H
   {"evdev", "Linux Event Device",
            "", M_evdev, I_empty, STD_FLG,
                        {0x00, 0x00, 0x00, 0x00} , 16, 16, 0, 0, NULL},
   {"evabs", "Linux Event Device - absolute mode",
            "", M_evabs, I_empty, STD_FLG,
                        {0x00, 0x00, 0x00, 0x00} , 16, 16, 0, 1, NULL},
#endif /* HAVE_LINUX_INPUT_H */
   {"exps2",   "IntelliMouse Explorer (ps2) - 3 buttons, wheel unused",
           "ExplorerPS/2", M_imps2, I_exps2, STD_FLG,
                                {0xc0, 0x00, 0x00, 0x00}, 4, 1, 0, 0, 0},
#ifdef HAVE_LINUX_JOYSTICK_H
   {"js",   "Joystick mouse emulation",
           "Joystick", M_js, NULL, 0,
                              {0xFC, 0x00, 0x00, 0x00}, 12, 12, 0, 0, 0},
#endif
   {"genitizer", "\"Genitizer\" tablet, in relative mode.",
           "", M_geni, I_serial, CS8|PARENB|PARODD,
                                {0x80, 0x80, 0x00, 0x00}, 3, 1, 0, 0, 0},
   {"gunze",  "Gunze touch-screens (only button-1 events, by now)",
           "", M_gunze, I_gunze, STD_FLG,
                                {0xF9, 0x50, 0xF0, 0x30}, 11, 1, 0, 1, NULL}, 
   {"imps2","Microsoft Intellimouse (ps2)-autodetect 2/3 buttons,wheel unused",
           "", M_imps2, I_imps2, STD_FLG,
                                {0xC0, 0x00, 0x00, 0x00}, 4, 1, 0, 0, R_imps2},
   {"logi", "Used in some Logitech devices (only serial).",
           "Logitech", M_logi, I_logi, CS8 | CSTOPB | STD_FLG,
                                {0xe0, 0x80, 0x80, 0x00}, 3, 3, 0, 0, 0},
   {"logim",  "Turn logitech into Mouse-Systems-Compatible.",
           "", M_logimsc, I_serial, CS8 | CSTOPB | STD_FLG,
                                {0xf8, 0x80, 0x00, 0x00}, 5, 1, 0, 0, 0},
   {"mm",   "MM series. Probably an old protocol...",
           "MMSeries", M_mm, I_serial, CS8 | PARENB|PARODD | STD_FLG,
                                {0xe0, 0x80, 0x80, 0x00}, 3, 1, 0, 0, 0},
   {"ms3", "Microsoft Intellimouse (serial) - 3 buttons, wheel unused",
           "", M_ms3, I_pnp, CS7 | STD_FLG,
                                {0xc0, 0x40, 0xc0, 0x00}, 4, 1, 0, 0, R_ms3},
   {"ms+", "Like 'ms', but allows dragging with the middle button.",
           "", M_ms_plus, I_serial, CS7 | STD_FLG,
                                {0x40, 0x40, 0x40, 0x00}, 3, 1, 0, 0, 0},
   {"ms+lr", "'ms+', but you can reset m by pressing lr (see man page).",
           "", M_ms_plus_lr, I_serial, CS7 | STD_FLG,
                                {0x40, 0x40, 0x40, 0x00}, 3, 1, 0, 0, 0},
   {"msc",  "Mouse-Systems-Compatible (5bytes). Most 3-button mice.",
           "MouseSystems", M_msc, I_serial, CS8 | CSTOPB | STD_FLG,
                                {0xf8, 0x80, 0x00, 0x00}, 5, 1, 0, 0, R_msc},
   {"mtouch",  "MicroTouch touch-screens (only button-1 events, by now)",
           "", M_mtouch, I_mtouch, STD_FLG,
                                {0x80, 0x80, 0x80, 0x00}, 5, 1, 0, 1, NULL}, 
   {"ncr",  "Ncr3125pen, found on some laptops",
           "", M_ncr, NULL, STD_FLG,
                                {0x08, 0x08, 0x00, 0x00}, 7, 7, 0, 1, 0},
   {"netmouse","Genius NetMouse (ps2) - 2 buttons and 2 buttons 'up'/'down'.", 
           "", M_netmouse, I_netmouse, CS7 | STD_FLG,
                                {0xc0, 0x00, 0x00, 0x00}, 4, 1, 0, 0, 0},
   {"pnp",  "Plug and pray. New mice may not run with '-t ms'.",
           "", M_bare, I_pnp, CS7 | STD_FLG,
                                {0x40, 0x40, 0x40, 0x00}, 3, 1, 0, 0, 0},
   {"ps2",  "Busmice of the ps/2 series. Most busmice, actually.",
           "PS/2", M_ps2, I_ps2, STD_FLG,
                                {0xc0, 0x00, 0x00, 0x00}, 3, 1, 0, 0, R_ps2},
   {"sun",  "'msc' protocol, but only 3 bytes per packet.",
           "", M_sun, I_serial, CS8 | CSTOPB | STD_FLG,
                                {0xf8, 0x80, 0x00, 0x00}, 3, 1, 0, 0, R_sun},
   {"summa",  "Summagraphics or Genius tablet absolute mode(MM-Series)",
           "", M_summa, I_summa, STD_FLG,
                                {0x80, 0x80, 0x00, 0x00}, 5, 1, 0, 1, R_summa},
   {"syn", "The \"Synaptics\" serial TouchPad.",
           "synaptics", M_synaptics_serial, I_serial, CS7 | STD_FLG,
                                {0x40, 0x40, 0x40, 0x00}, 6, 6, 1, 0, 0},
   {"synps2", "The \"Synaptics\" PS/2 TouchPad",
           "synaptics_ps2", M_synaptics_ps2, I_synps2, STD_FLG,
                                {0x80, 0x80, 0x00, 0x00}, 6, 1, 1, 0, 0},
   {"twid", "Twidddler keyboard",
           "", M_twid, I_twid, CS8 | STD_FLG,
                                {0x80, 0x00, 0x80, 0x80}, 5, 1, 0, 0, 0},
   {"vsxxxaa", "The DEC VSXXX-AA/GA serial mouse on DEC workstations.",
           "", M_vsxxx_aa, I_serial, CS8 | PARENB | PARODD | STD_FLG,
                                {0xe0, 0x80, 0x80, 0x00}, 3, 1, 0, 0, 0},
   {"wacom","Wacom Protocol IV Tablets: Pen+Mouse, relative+absolute mode",
           "", M_wacom, I_wacom, STD_FLG,
                                {0x80, 0x80, 0x80, 0x00}, 7, 1, 0, 0, 0},
   {"wp",   "Genius WizardPad tablet",
           "wizardpad", M_wp, I_wp, STD_FLG,
                                {0xFA, 0x42, 0x00, 0x00}, 10, 1, 0, 1, 0},
   {"",     "",
           "", NULL, NULL, 0,
                                {0x00, 0x00, 0x00, 0x00}, 0, 0, 0, 0, 0}
};

/*------------------------------------------------------------------------*/
/* and the help */

int M_listTypes(void)
{
   Gpm_Type *type;

   printf(GPM_MESS_VERSION "\n");
   printf(GPM_MESS_AVAIL_MYT);
   for (type=mice; type->fun; type++)
      printf(GPM_MESS_SYNONYM, type->repeat_fun?'*':' ',
      type->name, type->desc, type->synonyms);

   putchar('\n');

   return 1; /* to exit() */
}

/* indent: use three spaces. no tab. not two or four. three */

/* Local Variables: */
/* c-indent-level: 3 */
/* End: */
