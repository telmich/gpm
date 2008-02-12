/*
 * mice.c - mouse definitions for gpm-Linux
 *
 * Copyright (C) 1993        Andrew Haylett <ajh@gec-mrc.co.uk>
 * Copyright (C) 1994-2000   Alessandro Rubini <rubini@linux.it>
 * Copyright (C) 1998,1999   Ian Zimmerman <itz@rahul.net>
 * Copyright (C) 2001,2002   Nico Schottelius <nicos@pcsystems.de>
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
 


#include "headers/gpmInt.h"
#include "headers/twiddler.h"
#include "headers/synaptics.h"
#include "headers/message.h"

/*========================================================================*/
/* Parsing argv: helper dats struct function (should they get elsewhere?) */
/*========================================================================*/

enum argv_type {
   ARGV_BOOL = 1,
   ARGV_INT, /* "%i" */
   ARGV_DEC, /* "%d" */
   ARGV_STRING,
   /* other types must be added */
   ARGV_END = 0
};

typedef struct argv_helper {
   char *name;
   enum argv_type type;
   union u {
      int *iptr;   /* used for int and bool arguments */
      char **sptr; /* used for string arguments, by strdup()ing the value */
   } u;
   int value; /* used for boolean arguments */
} argv_helper;

static int parse_argv(argv_helper *info, int argc, char **argv)
{
   int i, j = 0, errors = 0;
   long l;
   argv_helper *p;
   char *s, *t;
   int base = 0; /* for strtol */


   for (i=1; i<argc; i++) {
      for (p = info; p->type != ARGV_END; p++) {
         j = strlen(p->name);
         if (strncmp(p->name, argv[i], j))
            continue;
         if (isalnum(argv[i][j]))
            continue;
         break;
      }
      if (p->type == ARGV_END) { /* not found */
         fprintf(stderr, "%s: Uknown option \"%s\" for pointer \"%s\"\n",
                  option.progname, argv[i], argv[0]);
         errors++;
         continue;
      }
      /* Found. Look for trailing stuff, if any */
      s = argv[i]+j;
      while (*s && isspace(*s)) s++; /* skip spaces */
      if (*s == '=') s++; /* skip equal */
      while (*s && isspace(*s)) s++; /* skip other spaces */

      /* Now parse what s is */
      switch(p->type) {
         case ARGV_BOOL:
            if (*s) {
               gpm_report(GPM_PR_ERR,GPM_MESS_OPTION_NO_ARG,option.progname,p->name,s);
               errors++;
            }
            *(p->u.iptr) = p->value;
            break;

         case ARGV_DEC:
            base = 10; /* and fall through */
         case ARGV_INT:
            l = strtol(s, &t, base);
            if (*t) {
               gpm_report(GPM_PR_ERR,GPM_MESS_INVALID_ARG, option.progname, s, p->name);
               errors++;
               break;
            }
            *(p->u.iptr) = (int)l;
            break;

         case ARGV_STRING:
            *(p->u.sptr) = strdup(s);
            break;

         case ARGV_END: /* let's please "-Wall" */
            break;
      }
   } /* for i in argc */
   if (errors) gpm_report(GPM_PR_ERR,GPM_MESS_CONT_WITH_ERR, option.progname);
   return errors;
}

/*========================================================================*/
/* Provide a common error engine by parsing with an empty option-set */
/*========================================================================*/
static volatile int check_no_argv(int argc, char **argv)
{
   static argv_helper optioninfo[] = {
      {"",       ARGV_END}
      };
   return parse_argv(optioninfo, argc, argv);
}

/*========================================================================*/
/* Parse the "old" -o options */
/*========================================================================*/
static int option_modem_lines(int fd, int argc, char **argv)
{
   static unsigned int err, lines, reallines;

   static argv_helper optioninfo[] = {
      {"dtr",  ARGV_BOOL, u: {iptr: &lines}, value: TIOCM_DTR},
      {"rts",  ARGV_BOOL, u: {iptr: &lines}, value: TIOCM_RTS},
      {"both", ARGV_BOOL, u: {iptr: &lines}, value: TIOCM_DTR | TIOCM_RTS},
      {"",       ARGV_END}
      };

   if (argc<2) return 0;
   if (argc > 2) {
      gpm_report(GPM_PR_ERR,GPM_MESS_TOO_MANY_OPTS,option.progname, argv[0]);
      errno = EINVAL; /* used by gpm_oops(), if the caller reports failure */
      return -1;
   }
   err = parse_argv(optioninfo, argc, argv);
   if(err) return 0; /* a message has been printed, but go on as good */

   /* ok, move the lines */
   ioctl(fd, TIOCMGET, &reallines);
   reallines &= ~lines;
   ioctl(fd, TIOCMSET, &reallines);
   return 0;
}

/*========================================================================*/
/*  real absolute coordinates for absolute devices,  not very clean */
/*========================================================================*/

#define REALPOS_MAX 16383 /*  min 0 max=16383, but due to change.  */
int realposx=-1, realposy=-1;

/*========================================================================*/
/* 
 * When repeating, it is important not to try to repeat more bits of dx and
 * dy than the protocol can handle.  Otherwise, you may end up repeating the
 * low bits of a large value, which causes erratic motion.
 */
/*========================================================================*/

static int limit_delta(int delta, int min, int max)
{
   return delta > max ? max :
          delta < min ? min : delta;
}

/*========================================================================*/
/*
 * Ok, here we are: first, provide the functions; initialization is later.
 * The return value is the number of unprocessed bytes
 */
/*========================================================================*/

#ifdef HAVE_LINUX_INPUT_H
static int M_evdev (Gpm_Event * state, unsigned char *data)
{
   struct input_event thisevent;
   (void) memcpy (&thisevent, data, sizeof (struct input_event));
   if (thisevent.type == EV_REL) {
      if (thisevent.code == REL_X)
         state->dx = (signed char) thisevent.value;
      else if (thisevent.code == REL_Y)
         state->dy = (signed char) thisevent.value;
   } else if (thisevent.type == EV_KEY) {
      switch(thisevent.code) {
         case BTN_LEFT:    state->buttons ^= GPM_B_LEFT;    break;
         case BTN_MIDDLE:  state->buttons ^= GPM_B_MIDDLE;  break;
         case BTN_RIGHT:   state->buttons ^= GPM_B_RIGHT;   break;
         case BTN_SIDE:    state->buttons ^= GPM_B_MIDDLE;  break;
      }   
   }
   return 0;
}
#endif /* HAVE_LINUX_INPUT_H */

static int M_ms(Gpm_Event *state,  unsigned char *data)
{
   /*
    * some devices report a change of middle-button state by
    * repeating the current button state  (patch by Mark Lord)
    */
   static unsigned char prev=0;

   if (data[0] == 0x40 && !(prev|data[1]|data[2]))
      state->buttons = GPM_B_MIDDLE; /* third button on MS compatible mouse */
   else
      state->buttons= ((data[0] & 0x20) >> 3) | ((data[0] & 0x10) >> 4);
   prev = state->buttons;
   state->dx=      (signed char)(((data[0] & 0x03) << 6) | (data[1] & 0x3F));
   state->dy=      (signed char)(((data[0] & 0x0C) << 4) | (data[2] & 0x3F));

   return 0;
}

static int M_ms_plus(Gpm_Event *state, unsigned char *data)
{
   static unsigned char prev=0;

   state->buttons= ((data[0] & 0x20) >> 3) | ((data[0] & 0x10) >> 4);
   state->dx=      (signed char)(((data[0] & 0x03) << 6) | (data[1] & 0x3F));
   state->dy=      (signed char)(((data[0] & 0x0C) << 4) | (data[2] & 0x3F));

   /* Allow motion *and* button change (Michael Plass) */

   if((state->dx==0) &&(state->dy==0) && (state->buttons==(prev&~GPM_B_MIDDLE)))
      state->buttons = prev^GPM_B_MIDDLE;  /* no move or change: toggle middle */
   else
      state->buttons |= prev&GPM_B_MIDDLE;    /* change: preserve middle */

   prev=state->buttons;

   return 0;
}

static int M_ms_plus_lr(Gpm_Event *state,  unsigned char *data)
{
   /*
    * Same as M_ms_plus but with an addition by Edmund GRIMLEY EVANS
    */
   static unsigned char prev=0;

   state->buttons= ((data[0] & 0x20) >> 3) | ((data[0] & 0x10) >> 4);
   state->dx=      (signed char)(((data[0] & 0x03) << 6) | (data[1] & 0x3F));
   state->dy=      (signed char)(((data[0] & 0x0C) << 4) | (data[2] & 0x3F));

   /* Allow motion *and* button change (Michael Plass) */

   if((state->dx==0)&& (state->dy==0) && (state->buttons==(prev&~GPM_B_MIDDLE)))
      state->buttons = prev^GPM_B_MIDDLE; /* no move or change: toggle middle */
   else
      state->buttons |= prev&GPM_B_MIDDLE;/* change: preserve middle */

   /* Allow the user to reset state of middle button by pressing
      the other two buttons at once (Edmund GRIMLEY EVANS) */

   if (!((~state->buttons)&(GPM_B_LEFT|GPM_B_RIGHT)) &&
      ((~prev)&(GPM_B_LEFT|GPM_B_RIGHT)))
      state->buttons &= ~GPM_B_MIDDLE;

   prev=state->buttons;

   return 0;
}

/* Summagraphics/Genius/Acecad tablet support absolute mode*/
/* Summagraphics MM-Series format*/
/* (Frank Holtz) hof@bigfoot.de Tue Feb 23 21:04:09 MET 1999 */
int SUMMA_BORDER=100;
int summamaxx,summamaxy;
char summaid=-1;
static int M_summa(Gpm_Event *state, unsigned char *data)
{
   int x, y;

   x = ((data[2]<<7) | data[1])-SUMMA_BORDER;
   if (x<0) x=0;
   if (x>summamaxx) x=summamaxx;
   state->x = (x * win.ws_col / summamaxx);
   realposx = (x * 16383 / summamaxx);

   y = ((data[4]<<7) | data[3])-SUMMA_BORDER;
   if (y<0) y=0; if (y>summamaxy) y=summamaxy;
   state->y = 1 + y * (win.ws_row-1)/summamaxy;
   realposy = y * 16383 / summamaxy;  

   state->buttons=
    !!(data[0]&1) * GPM_B_LEFT +
    !!(data[0]&2) * GPM_B_RIGHT +
    !!(data[0]&4) * GPM_B_MIDDLE;

   return 0;
}

/* ps2 */
static int R_ps2(Gpm_Event *state, int fd)
{
   signed char buffer[3];

   buffer[0]=((state->buttons & GPM_B_LEFT  ) > 0)*1 +
             ((state->buttons & GPM_B_RIGHT ) > 0)*2 +
             ((state->buttons & GPM_B_MIDDLE) > 0)*4;
   buffer[0] |= 8 +
             ((state->dx < 0) ? 0x10 : 0) +
             ((state->dy > 0) ? 0x20 : 0);
   buffer[1] = limit_delta(state->dx, -128, 127);
   buffer[2] = limit_delta(-state->dy, -128, 127);
   return write(fd,buffer,3);
}


/* Thu Jan 28 20:54:47 MET 1999 hof@hof-berlin.de SummaSketch reportformat */
static int R_summa(Gpm_Event *state, int fd)
{
   signed char buffer[5];
   static int x,y;
  
   if (realposx==-1) { /* real absolute device? */
      x=x+(state->dx*25); if (x<0) x=0; if (x>16383) x=16383;
      y=y+(state->dy*25); if (y<0) y=0; if (y>16383) y=16383;
   } else { /* yes */
      x=realposx; y=realposy;
   }

   buffer[0]=0x98+((state->buttons&GPM_B_LEFT)>0?1:0)+
                  ((state->buttons&GPM_B_MIDDLE)>0?2:0)+
                  ((state->buttons&GPM_B_RIGHT)>0?3:0);
   buffer[1]=x & 0x7f;  /* X0-6*/
   buffer[2]=(x >> 7) & 0x7f;  /* X7-12*/
   buffer[3]=y & 0x7f;  /* Y0-6*/
   buffer[4]=(y >> 7) & 0x7f;  /* Y7-12*/

   return write(fd,buffer,5);
}



/* 'Genitizer' (kw@dtek.chalmers.se 11/12/97) */
static int M_geni(Gpm_Event *state,  unsigned char *data)
{
   /* this is a little confusing. If we use the stylus, we
    * have three buttons (tip, lower, upper), and if
    * we use the puck we have four buttons. (And the
    * protocol is a little mangled if several of the buttons
    * on the puck are pressed simultaneously. 
    * I don't use the puck, hence I try to decode three buttons
    * only. tip = left, lower = middle, upper = right
    */
   state->buttons = 
      (data[0] & 0x01)<<2 | 
      (data[0] & 0x02) |
      (data[0] & 0x04)>>2;

   state->dx = ((data[1] & 0x3f) ) * ((data[0] & 0x10)?1:-1);   
   state->dy = ((data[2] & 0x3f) ) * ((data[0] & 0x8)?-1:1);

   return 0;
}


/* m$ 'Intellimouse' (steveb 20/7/97) */
static int M_ms3(Gpm_Event *state,  unsigned char *data)
{
   state->wdx = state->wdy = 0;
   state->buttons= ((data[0] & 0x20) >> 3)   /* left */
      | ((data[3] & 0x10) >> 3)   /* middle */
      | ((data[0] & 0x10) >> 4)   /* right */
      | (((data[3] & 0x0f) == 0x0f) * GPM_B_UP)     /* wheel up */
      | (((data[3] & 0x0f) == 0x01) * GPM_B_DOWN);  /* wheel down */
   state->dx = (signed char) (((data[0] & 0x03) << 6) | (data[1] & 0x3F));
   state->dy = (signed char) (((data[0] & 0x0C) << 4) | (data[2] & 0x3F));

   switch (data[3] & 0x0f) {
      case 0x0e: state->wdx = +1; break;
      case 0x02: state->wdx = -1; break;
      case 0x0f: state->wdy = +1; break;
      case 0x01: state->wdy = -1; break;
   }

   return 0;
}

static int R_ms3(Gpm_Event *state, int fd)
{

   int dx, dy;
   char buf[4] = {0, 0, 0, 0};
   
   buf[0] |= 0x40;
   
   if (state->buttons & GPM_B_LEFT)     buf[0] |= 0x20;
   if (state->buttons & GPM_B_MIDDLE)   buf[3] |= 0x10;
   if (state->buttons & GPM_B_RIGHT)    buf[0] |= 0x10;
   if (state->buttons & GPM_B_UP)       buf[3] |= 0x0f;
   if (state->buttons & GPM_B_DOWN)     buf[3] |= 0x01;

   dx = limit_delta(state->dx, -128, 127);
   buf[1]   = dx & ~0xC0;
   buf[0]  |= (dx & 0xC0) >> 6;

   dy = limit_delta(state->dy, -128, 127);
   buf[2] = dy & ~0xC0;
   buf[0] |= (dy & 0xC0) >> 4;

   /* wheel */
   if       (state->wdy > 0)   buf[3] |= 0x0f;
   else if  (state->wdy < 0)   buf[3] |= 0x01;
   
   return write(fd,buf,4);
}

/* M_brw is a variant of m$ 'Intellimouse' the middle button is different */
static int M_brw(Gpm_Event *state,  unsigned char *data)
{
   state->buttons= ((data[0] & 0x20) >> 3)   /* left */
      | ((data[3] & 0x20) >> 4)   /* middle */
      | ((data[0] & 0x10) >> 4);   /* right */
   state->dx=      (signed char)(((data[0] & 0x03) << 6) | (data[1] & 0x3F));
   state->dy=      (signed char)(((data[0] & 0x0C) << 4) | (data[2] & 0x3F));
   if (((data[0]&0xC0) != 0x40)||
      ((data[1]&0xC0) != 0x00)||
      ((data[2]&0xC0) != 0x00)||
      ((data[3]&0xC0) != 0x00)) {
      gpm_report(GPM_PR_DEBUG,GPM_MESS_SKIP_DATAP,data[0],data[1],data[2],data[3]);
      return -1;
   }
   /* wheel (dz) is (data[3] & 0x0f) */
   /* where is the side button? I can sort of detect it at 9600 baud */
   /* Note this mouse is very noisy */

   return 0;
}

static int M_bare(Gpm_Event *state,  unsigned char *data)
{
   /* a bare ms protocol */
   state->buttons= ((data[0] & 0x20) >> 3) | ((data[0] & 0x10) >> 4);
   state->dx=      (signed char)(((data[0] & 0x03) << 6) | (data[1] & 0x3F));
   state->dy=      (signed char)(((data[0] & 0x0C) << 4) | (data[2] & 0x3F));
   return 0;
}

static int M_sun(Gpm_Event *state,  unsigned char *data)
{
   state->buttons= (~data[0]) & 0x07;
   state->dx=      (signed char)(data[1]);
   state->dy=     -(signed char)(data[2]);
   return 0;
}

static int M_msc(Gpm_Event *state,  unsigned char *data)
{
   state->buttons= (~data[0]) & 0x07;
   state->dx=      (signed char)(data[1]) + (signed char)(data[3]);
   state->dy=     -((signed char)(data[2]) + (signed char)(data[4]));
   return 0;
}

/* itz Mon Jan 11 23:51:38 PST 1999 this code moved here from gpm.c */
/* (processMouse) */

static int R_msc(Gpm_Event *state, int fd)
{
   signed char buffer[5];
   int dx, dy;

   /* sluggish... */
   buffer[0]=(state->buttons ^ 0x07) | 0x80;
   dx = limit_delta(state->dx, -256, 254);
   buffer[3] =  state->dx - (buffer[1] = state->dx/2); /* Markus */
   dy = limit_delta(state->dy, -256, 254);
   buffer[4] = -state->dy - (buffer[2] = -state->dy/2);
   return write(fd,buffer,5);

}
                       
static int R_imps2(Gpm_Event *state, int fd)
{
   signed char buffer[4];
   int dx, dy;

   dx = limit_delta(state->dx, -256, 255);
   dy = limit_delta(state->dy, -256, 255);

   buffer[0] = 8 |
      (state->buttons & GPM_B_LEFT ? 1 : 0) |
      (state->buttons & GPM_B_MIDDLE ? 4 : 0) |
      (state->buttons & GPM_B_RIGHT ? 2 : 0) | 
      (dx < 0 ? 0x10 : 0) |
      (dy > 0 ? 0x20 : 0);
   buffer[1] = dx & 0xFF;
   buffer[2] = (-dy) & 0xFF;
   buffer[3] = 
      (state->buttons & GPM_B_UP ? -1 : 0) + 
      (state->buttons & GPM_B_DOWN ? 1 : 0);
  
   return write(fd,buffer,4);

}

static int M_logimsc(Gpm_Event *state,  unsigned char *data) /* same as msc */
{
   state->buttons= (~data[0]) & 0x07;
   state->dx=      (signed char)(data[1]) + (signed char)(data[3]);
   state->dy=     -((signed char)(data[2]) + (signed char)(data[4]));
   return 0;
}

static int M_mm(Gpm_Event *state,  unsigned char *data)
{
   state->buttons= data[0] & 0x07;
   state->dx=      (data[0] & 0x10) ?   data[1] : - data[1];
   state->dy=      (data[0] & 0x08) ? - data[2] :   data[2];
   return 0;
}

static int M_logi(Gpm_Event *state,  unsigned char *data) /* equal to mm */
{
   state->buttons= data[0] & 0x07;
   state->dx=      (data[0] & 0x10) ?   data[1] : - data[1];
   state->dy=      (data[0] & 0x08) ? - data[2] :   data[2];
   return 0;
}

static int M_bm(Gpm_Event *state,  unsigned char *data) /* equal to sun */
{
   state->buttons= (~data[0]) & 0x07;
   state->dx=      (signed char)data[1];
   state->dy=     -(signed char)data[2];
   return 0;
}

static int M_ps2(Gpm_Event *state,  unsigned char *data)
{
   static int tap_active=0; /* there exist glidepoint ps2 mice */

   state->buttons=
      !!(data[0]&1) * GPM_B_LEFT +
      !!(data[0]&2) * GPM_B_RIGHT +
      !!(data[0]&4) * GPM_B_MIDDLE;

   if (data[0]==0 && opt_glidepoint_tap) /* by default this is false */
      state->buttons = tap_active = opt_glidepoint_tap;
   else if (tap_active) {
      if (data[0]==8)
         state->buttons = tap_active = 0;
      else
         state->buttons = tap_active;
   }  

   /* Some PS/2 mice send reports with negative bit set in data[0]
    * and zero for movement.  I think this is a bug in the mouse, but
    * working around it only causes artifacts when the actual report is -256;
    * they'll be treated as zero. This should be rare if the mouse sampling
    * rate is set to a reasonable value; the default of 100 Hz is plenty.
    * (Stephen Tell)
    */
   if(data[1] != 0)
      state->dx=   (data[0] & 0x10) ? data[1]-256 : data[1];
   else
      state->dx = 0;
   if(data[2] != 0)
      state->dy= -((data[0] & 0x20) ? data[2]-256 : data[2]);
   else
      state->dy = 0;
   return 0;
}

static int M_imps2(Gpm_Event *state,  unsigned char *data)
{
   
   static int tap_active=0;         /* there exist glidepoint ps2 mice */
   state->wdx  = state->wdy   = 0;  /* Clear them.. */
   state->dx   = state->dy    = state->wdx = state->wdy = 0;

   state->buttons= ((data[0] & 1) << 2)   /* left              */
      | ((data[0] & 6) >> 1);             /* middle and right  */
   
   if (data[0]==0 && opt_glidepoint_tap) // by default this is false
      state->buttons = tap_active = opt_glidepoint_tap;
   else if (tap_active) {
      if (data[0]==8)
         state->buttons = tap_active = 0;
      else state->buttons = tap_active;
   }

   /* Standard movement.. */
   state->dx = (data[0] & 0x10) ? data[1] - 256 : data[1];
   state->dy = (data[0] & 0x20) ? -(data[2] - 256) : -data[2];
   
   /* The wheels.. */
   switch (data[3] & 0x0f) {
      case 0x0e: state->wdx = +1; break;
      case 0x02: state->wdx = -1; break;
      case 0x0f: state->wdy = +1; break;
      case 0x01: state->wdy = -1; break;
   }
   
   /* old code:
      - did it signed: 
   state->buttons |= (data[3]<0) * GPM_B_UP + (data[3]>0) * GPM_B_DOWN;
      - and unsigned:
   state->buttons |= (data[3]>127) * GPM_B_UP + (data[3]<127) * GPM_B_DOWN;
   */
    
   return 0;

}

static int M_netmouse(Gpm_Event *state,  unsigned char *data)
{
   /* Avoid these beasts if you can.  They connect to normal PS/2 port,
    * but their protocol is one byte longer... So if you have notebook
    * (like me) with internal PS/2 mouse, it will not work
    * together. They have four buttons, but two middle buttons can not
    * be pressed simultaneously, and two middle buttons do not send
    * 'up' events (however, they autorepeat...)

    * Still, you might want to run this mouse in plain PS/2 mode -
    * where it behaves correctly except that middle 2 buttons do
    * nothing.

    * Protocol is
    * 3 bytes like normal PS/2
    * 4th byte: 0xff button 'down', 0x01 button 'up'
    * [this is so braindamaged that it *must* be some kind of
    * compatibility glue...]

    * Pavel Machek <pavel@ucw.cz>
    */

   state->buttons=
      !!(data[0]&1) * GPM_B_LEFT +
      !!(data[0]&2) * GPM_B_RIGHT +
      !!(data[3]) * GPM_B_MIDDLE;

   if(data[1] != 0)
      state->dx=   (data[0] & 0x10) ? data[1]-256 : data[1];
   else
      state->dx = 0;
   if(data[2] != 0)
      state->dy= -((data[0] & 0x20) ? data[2]-256 : data[2]);
   else
      state->dy = 0;
   return 0;
}

/* standard ps2 */
static Gpm_Type *I_ps2(int fd, unsigned short flags,
        struct Gpm_Type *type, int argc, char **argv)
{
   static unsigned char s[] = { 246, 230, 244, 243, 100, 232, 3, };
   write (fd, s, sizeof (s));
   usleep (30000);
   tcflush (fd, TCIFLUSH);
   return type;
}

static Gpm_Type *I_netmouse(int fd, unsigned short flags,
         struct Gpm_Type *type, int argc, char **argv)
{
   unsigned char magic[6] = { 0xe8, 0x03, 0xe6, 0xe6, 0xe6, 0xe9 };
   int i;

   if (check_no_argv(argc, argv)) return NULL;
   for (i=0; i<6; i++) {
      unsigned char c = 0;
      write( fd, magic+i, 1 );
      read( fd, &c, 1 );
      if (c != 0xfa) {
         gpm_report(GPM_PR_ERR,GPM_MESS_NETM_NO_ACK,c);
         return NULL;
      }
   }
   {
      unsigned char rep[3] = { 0, 0, 0 };
      read( fd, rep, 1 );
      read( fd, rep+1, 1 );
      read( fd, rep+2, 1 );
      if (rep[0] || (rep[1] != 0x33) || (rep[2] != 0x55)) {
         gpm_report(GPM_PR_ERR,GPM_MESS_NETM_INV_MAGIC, rep[0], rep[1], rep[2]);
         return NULL;
      }
   }
   return type;
}

#define GPM_B_BOTH (GPM_B_LEFT|GPM_B_RIGHT)
static int M_mman(Gpm_Event *state,  unsigned char *data)
{
   /*
    * the damned MouseMan has 3/4 bytes packets. The extra byte 
    * is only there if the middle button is active.
    * I get the extra byte as a packet with magic numbers in it.
    * and then switch to 4-byte mode.
    */
    static unsigned char prev=0;
    static Gpm_Type *mytype=mice; /* it is the first */
    unsigned char b = (*data>>4);

   if(data[1]==GPM_EXTRA_MAGIC_1 && data[2]==GPM_EXTRA_MAGIC_2) {
      /* got unexpected fourth byte */
      if (b > 0x3) return -1;  /* just a sanity check */
      //if ((b=(*data>>4)) > 0x3) return -1;  /* just a sanity check */
      state->dx=state->dy=0;
    
      mytype->packetlen=4;
      mytype->getextra=0;
   }
   else {
      /* got 3/4, as expected */
    
      /* motion is independent of packetlen... */
      state->dx= (signed char)(((data[0] & 0x03) << 6) | (data[1] & 0x3F));
      state->dy= (signed char)(((data[0] & 0x0C) << 4) | (data[2] & 0x3F));
    
      prev= ((data[0] & 0x20) >> 3) | ((data[0] & 0x10) >> 4);
      if (mytype->packetlen==4) b=data[3]>>4;
   }
     
   if(mytype->packetlen==4) {
      if(b == 0) {
         mytype->packetlen=3;
         mytype->getextra=1;
      } else {
         if (b & 0x2) prev |= GPM_B_MIDDLE;
         if (b & 0x1) prev |= opt_glidepoint_tap;
      }
   }
   state->buttons=prev;

   /* This "chord-middle" behaviour was reported by David A. van Leeuwen */
   if (((prev^state->buttons) & GPM_B_BOTH)==GPM_B_BOTH )
      state->buttons = state->buttons ? GPM_B_MIDDLE : 0;
   prev=state->buttons;
  
   return 0;
}

/* 
 *  Wacom Tablets with pen and mouse:
 *  Relative-Mode And Absolute-Mode; 
 *  Stefan Runkel  01/2000 <runkel@runkeledv.de>,
 *  Mike Pioskowik 01/2000 <pio.d1mp@nbnet.de>
*/
/* for relative and absolute : */
int WacomModell =-1;          /* -1 means "dont know" */
int WacomAbsoluteWanted=0;    /* Tell Driver if Relative or Absolute */
int wmaxx, wmaxy;         
char upmbuf[25]; /* needed only for macro buttons of ultrapad */

/* Data for Wacom Modell Identification */
/* (MaxX, MaxY are for Modells which do not answer resolution requests */
struct WC_MODELL{
   char name[15];       
   char magic[3];
   int  maxX;
   int  maxY;
   int  border;
   int  treshold;
} wcmodell[] = {
   /* ModellName    Magic     MaxX     MaxY  Border  Tresh */
    { "UltraPad"  , "UD",        0,       0,    250,    20 }, 
 /* { "Intuos"    , "GD",        0,       0,      0,    20 }, not supported */
    { "PenPartner", "CT",        0,       0,      0,    20 }, 
    { "Graphire"  , "ET",     5103,    3711,      0,    20 }
   };

#define IsA(m) ((WacomModell==(-1))? 0:!strcmp(#m,wcmodell[WacomModell].name))

static int M_wacom(Gpm_Event *state, unsigned char *data)
{
   static int ox=-1, oy;
   int x, y;
   int macro=0;      /* macro buttons from tablet */

   /* Bit [0]&64 seems to have different meanings -
    *   graphire:   inside of active area;
    *   Ultrapad:   Pen in proximity (means: pen detected)
    *   Penpartner: no idea...
    * this may be of worth sometimes, but for now, we can say, that
    * if the graphire tells us that we are in the active area, we can tell
    * also that the graphire has the pen in proximity.
    */

   if (!(data[0]&64)) { /* Tool not in proximity or out of active area */
   /* handle Ultrapad macro buttons, if we get a packet without
    * proximity bit but with the buttonflag set, we know that we have
    * a macro event */
      if (IsA(UltraPad)) {
         if (data[0]&8) { /* here: macro button has been pressed */
            if (data[3]&8)  macro=(data[6]); 
            if (data[3]&16) macro=(data[6])+12;
            if (data[3]&32) macro=(data[6])+24;  /* rom-version >= 1.3 */

            state->modifiers=macro;
            /* Here we simulate the middle mousebutton */
            /* with ultrapad Eprom Version 1.2 */
            /* WHY IS THE FOLLOWING CODE DISABLE ? FIXME
            gpm_report(GPM_PR_INFO,GPM_MESS_WACOM_MACRO, macro); 
            if (macro==12) state->buttons =  GPM_B_MIDDLE;
            */
         } /* end if macrobutton pressed */
      } /* end if ultrapad */
          
      if (!IsA(UltraPad)){ /* Tool out of active area */ 
         ox=-1; 
         state->buttons=0; 
         state->dx=state->dy=0; 
      }

      return 0; /* nothing more to do so leave */
   } /* end if Tool out of active area*/ 

   x = (((data[0] & 0x3) << 14) +  (data[1] << 7) + data[2]); 
   y = (((data[3] & 0x3) << 14) + (data[4] << 7) + data[5]);
   
   if (WacomAbsoluteWanted) { /* Absolute Mode */
      if (x>wmaxx) x=wmaxx; if (x<0) x=0;
      if (y>wmaxy) y=wmaxy; if (y<0) y=0;
      state->x  = (x * win.ws_col / wmaxx);
      state->y  = (y * win.ws_row / wmaxy);
       
      realposx = (x / wmaxx); /* this two lines come from the summa driver. */
      realposy = (y / wmaxy); /* they seem to be buggy (always give zero).  */

   } else {                                               /* Relative Mode */
      /* Treshold; if greather then treat tool as first time in proximity */
      if( abs(x-ox)>(wmaxx/wcmodell[WacomModell].treshold) 
       || abs(y-oy)>(wmaxy/wcmodell[WacomModell].treshold) ) ox=x; oy=y;

      state->dx= (x-ox) / (wmaxx / win.ws_col / wcmodell[WacomModell].treshold);
      state->dy= (y-oy) / (wmaxy / win.ws_row / wcmodell[WacomModell].treshold);
   }

   ox=x; oy=y;    
   
   state->buttons=                      /* for Ultra-Pad and graphire */
      !!(data[3]&8)  * GPM_B_LEFT  +
      !!(data[3]&16) * GPM_B_RIGHT +
      !!(data[3]&32) * GPM_B_MIDDLE;   /* UD: rom-version >=1.3 */
   return 0;
}

/* Calcomp UltraSlate tablet (John Anderson)
 * modeled after Wacom and NCR drivers (thanks, guys)
 * Note: I don't know how to get the tablet size from the tablet yet, so
 * these defines will have to do for now.
 */

#define CAL_LIMIT_BRK 255

#define CAL_X_MIN 0x40
#define CAL_X_MAX 0x1340
#define CAL_X_SIZE (CAL_X_MAX - CAL_X_MIN)

#define CAL_Y_MIN 0x40
#define CAL_Y_MAX 0xF40
#define CAL_Y_SIZE (CAL_Y_MAX - CAL_Y_MIN)

static int M_calus(Gpm_Event *state, unsigned char *data)
{
   int x, y;

   x = ((data[1] & 0x3F)<<7) | (data[2] & 0x7F);
   y = ((data[4] & 0x1F)<<7) | (data[5] & 0x7F);

   state->buttons = GPM_B_LEFT * ((data[0]>>2) & 1)
     + GPM_B_MIDDLE * ((data[0]>>3) & 1)
     + GPM_B_RIGHT * ((data[0]>>4) & 1);

   state->dx = 0; state->dy = 0;

   state->x = x < CAL_X_MIN ? 0
     : x > CAL_X_MAX ? win.ws_col+1
     : (long)(x-CAL_X_MIN) * (long)(win.ws_col-1) / CAL_X_SIZE+2;

   state->y = y < CAL_Y_MIN ? win.ws_row + 1
     : y > CAL_Y_MAX ? 0
     : (long)(CAL_Y_MAX-y) * (long)win.ws_row / CAL_Y_SIZE + 1;

   realposx = x < CAL_X_MIN ? 0
     : x > CAL_X_MAX ? 16384
     : (long)(x-CAL_X_MIN) * (long)(16382) / CAL_X_SIZE+2;

   realposy = y < CAL_Y_MIN ? 16384
     : y > CAL_Y_MAX ? 0
     : (long)(CAL_Y_MAX-y) * (long)16383 / CAL_Y_SIZE + 1;

   return 0;
}

static int M_calus_rel(Gpm_Event *state, unsigned char *data)
{
   static int ox=-1, oy;
   int x, y;

   x = ((data[1] & 0x3F)<<7) | (data[2] & 0x7F);
   y = ((data[4] & 0x1F)<<7) | (data[5] & 0x7F);

   if (ox==-1 || abs(x-ox)>CAL_LIMIT_BRK || abs(y-oy)>CAL_LIMIT_BRK) {
      ox=x; oy=y;
   }

   state->buttons = GPM_B_LEFT * ((data[0]>>2) & 1)
     + GPM_B_MIDDLE * ((data[0]>>3) & 1)
     + GPM_B_RIGHT * ((data[0]>>4) & 1);

   state->dx = (x-ox)/5; state->dy = (oy-y)/5;
   ox=x; oy=y;
   return 0;
}



/* ncr pen support (Marc Meis) */

#define NCR_LEFT_X     40
#define NCR_RIGHT_X    2000

#define NCR_BOTTOM_Y   25
#define NCR_TOP_Y      1490

#define NCR_DELTA_X    (NCR_RIGHT_X - NCR_LEFT_X)
#define NCR_DELTA_Y    (NCR_TOP_Y - NCR_BOTTOM_Y)

static int M_ncr(Gpm_Event *state,  unsigned char *data)
{
   int x,y;

   state->buttons= (data[0]&1)*GPM_B_LEFT +
                !!(data[0]&2)*GPM_B_RIGHT;

   state->dx = (signed char)data[1]; /* currently unused */
   state->dy = (signed char)data[2];

   x = ((int)data[3] << 8) + (int)data[4];
   y = ((int)data[5] << 8) + (int)data[6];

   /* these formulaes may look curious, but this is the way it works!!! */

   state->x = x < NCR_LEFT_X
             ? 0
             : x > NCR_RIGHT_X
               ? win.ws_col+1
               : (long)(x-NCR_LEFT_X) * (long)(win.ws_col-1) / NCR_DELTA_X+2;

   state->y = y < NCR_BOTTOM_Y
             ? win.ws_row + 1
             : y > NCR_TOP_Y
          ? 0
          : (long)(NCR_TOP_Y-y) * (long)win.ws_row / NCR_DELTA_Y + 1;

   realposx = x < NCR_LEFT_X
             ? 0
             : x > NCR_RIGHT_X
               ? 16384
               : (long)(x-NCR_LEFT_X) * (long)(16382) / NCR_DELTA_X+2;

   realposy = y < NCR_BOTTOM_Y
             ? 16384
             : y > NCR_TOP_Y
          ? 0
          : (long)(NCR_TOP_Y-y) * (long)16383 / NCR_DELTA_Y + 1;

   return 0;
}

static int M_twid(Gpm_Event *state,  unsigned char *data)
{
   unsigned long message=0UL; int i,h,v;
   static int lasth, lastv, lastkey, key, lock=0, autorepeat=0;

   /* build the message as a single number */
   for (i=0; i<5; i++)
      message |= (data[i]&0x7f)<<(i*7);
   key = message & TW_ANY_KEY;

   if ((message & TW_MOD_M) == 0) { /* manage keyboard */
      if (((message & TW_ANY_KEY) != lastkey) || autorepeat)
         autorepeat = twiddler_key(message);
      lastkey = key;
      lock = 0; return -1; /* no useful mouse data */
   } 

   switch (message & TW_ANY1) {
      case TW_L1: state->buttons = GPM_B_RIGHT;  break;
      case TW_M1: state->buttons = GPM_B_MIDDLE; break;
      case TW_R1: state->buttons = GPM_B_LEFT;   break;
      case     0: state->buttons = 0;            break;
    }
   /* also, allow R1 R2 R3 (or L1 L2 L3) to be used as mouse buttons */
   if (message & TW_ANY2)  state->buttons |= GPM_B_MIDDLE;
   if (message & TW_L3)    state->buttons |= GPM_B_LEFT;
   if (message & TW_R3)    state->buttons |= GPM_B_RIGHT;

   /* put in modifiers information */
   {
      struct {unsigned long in, out;} *ptr, list[] = {
         { TW_MOD_S,  1<<KG_SHIFT },
         { TW_MOD_C,  1<<KG_CTRL  },
         { TW_MOD_A,  1<<KG_ALT   },
         { 0,         0}
      };
      for (ptr = list; ptr->in; ptr++)
         if(message & ptr->in) state->modifiers |= ptr->out;
   }

   /* now extraxt H/V */
   h = (message >> TW_H_SHIFT) & TW_M_MASK;
   v = (message >> TW_V_SHIFT) & TW_M_MASK;
   if (h & TW_M_BIT) h = -(TW_M_MASK + 1 - h);
   if (v & TW_M_BIT) v = -(TW_M_MASK + 1 - v);
  
#ifdef TWIDDLER_STATIC
   /* static implementation: return movement */
   if (!lock) {
      lasth = h;
      lastv = v;
      lock = 1;
   } 
   state->dx = -(h-lasth); lasth = h;
   state->dy = -(v-lastv); lastv = v; 

#elif defined(TWIDDLER_BALLISTIC)
   {
      /* in case I'll change the resolution */
      static int tw_threshold = 5;  /* above this it moves */
      static int tw_scale = 5;      /* every 5 report one */

      if (h > -tw_threshold && h < tw_threshold) state->dx=0;
      else {
         h = h - (h<0)*tw_threshold +lasth;
         lasth = h%tw_scale;
         state->dx = -(h/tw_scale);
      }
      if (v > -tw_threshold && v < tw_threshold) state->dy=0;
      else {
         v = v - (v<0)*tw_threshold +lastv;
         lastv = v%tw_scale;
         state->dy = -(v/tw_scale);
      }
   }

#else /* none defined: use mixed approach */
   {
      /* in case I'll change the resolution */
      static int tw_threshold = 60;    /* above this, movement is ballistic */
      static int tw_scale = 10;         /* ball: every 6 units move one unit */
      static int tw_static_scale = 3;  /* stat: every 3 units move one unit */
      static int lasthrest, lastvrest; /* integral of small motions uses rest */

      if (!lock) {
         lasth = h; lasthrest = 0;
         lastv = v; lastvrest = 0;
         lock = 1;
      } 

      if (h > -tw_threshold && h < tw_threshold) {
         state->dx = -(h-lasth+lasthrest)/tw_static_scale;
         lasthrest =  (h-lasth+lasthrest)%tw_static_scale;
      } else /* ballistic */ {
         h = h - (h<0)*tw_threshold + lasthrest;
         lasthrest = h%tw_scale;
         state->dx = -(h/tw_scale);
      }
      lasth = h;
      if (v > -tw_threshold && v < tw_threshold) {
         state->dy = -(v-lastv+lastvrest)/tw_static_scale;
         lastvrest =  (v-lastv+lastvrest)%tw_static_scale;
      } else /* ballistic */ {
         v = v - (v<0)*tw_threshold + lastvrest;
         lastvrest = v%tw_scale;
         state->dy = -(v/tw_scale);
      }
      lastv = v;
   }
#endif

   /* fprintf(stderr,"%4i %4i -> %3i %3i\n",h,v,state->dx,state->dy); */
   return 0;
}

#ifdef HAVE_LINUX_JOYSTICK_H
/* Joystick mouse emulation (David Given) */

static int M_js(Gpm_Event *state,  unsigned char *data)
{
   struct JS_DATA_TYPE *jdata = (void*)data;
   static int centerx = 0;
   static int centery = 0;
   static int oldbuttons = 0;
   static int count = 0;
   int dx;
   int dy;

   count++;
   if (count < 200) {
      state->buttons = oldbuttons;
      state->dx = 0;
      state->dy = 0;
      return 0;
   }
   count = 0;

   if (centerx == 0) {
      centerx = jdata->x;
      centery = jdata->y;
   }

   state->buttons = ((jdata->buttons & 1) * GPM_B_LEFT) |
                    ((jdata->buttons & 2) * GPM_B_RIGHT);
   oldbuttons     = state->buttons;

   dx             = (jdata->x - centerx) >> 6;
   dy             = (jdata->y - centery) >> 6;

   if (dx > 0) state->dx =   dx * dx;
   else        state->dx = -(dx * dx);
   state->dx >>= 2;

   if (dy > 0) state->dy = dy * dy;
   else        state->dy = -(dy * dy);
   state->dy >>= 2;

   /* Prevent pointer drift. (PC joysticks are notoriously inaccurate.) */

   if ((state->dx >= -1) && (state->dx <= 1)) state->dx = 0;
   if ((state->dy >= -1) && (state->dy <= 1)) state->dy = 0;

   return 0;
}
#endif /* have joystick.h */

/* Synaptics TouchPad mouse emulation (Henry Davies) */
static int M_synaptics_serial(Gpm_Event *state,  unsigned char *data)
{
   syn_process_serial_data (state, data);
   return 0;
}


/* Synaptics TouchPad mouse emulation (Henry Davies) */
static int M_synaptics_ps2(Gpm_Event *state,  unsigned char *data)
{
   syn_process_ps2_data(state, data);
   return 0;
}

static int M_mtouch(Gpm_Event *state,  unsigned char *data)
{
   /*
    * This is a simple decoder for the MicroTouch touch screen
    * devices. It uses the "tablet" format and only generates button-1
    * events. Check README.microtouch for additional information.
    */
   int x, y;
   static int avgx=-1, avgy;       /* average over time, for smooth feeling */
   static int upx, upy;            /* keep track of last finger-up place */
   static struct timeval uptv, tv; /* time of last up, and down events */

   #define REAL_TO_XCELL(x) (x * win.ws_col / 0x3FFF)
   #define REAL_TO_YCELL(y) (y * win.ws_row / 0x3FFF)

   #define GET_TIME(tv) (gettimeofday(&tv, (struct timezone *)NULL))
   #define DIF_TIME(t1,t2) ((t2.tv_sec -t1.tv_sec) *1000+ \
                           (t2.tv_usec-t1.tv_usec)/1000)

   if (!(data[0]&0x40)) {
    /*
     * finger-up event: this is usually offset a few pixels,
     * so ignore this x and y values. And invalidate avg.
     */
      upx = avgx;
      upy = avgy;
      GET_TIME(uptv); /* ready for the next finger-down */
      tv.tv_sec = 0;
      state->buttons = 0;
      avgx=-1; /* invalidate avg */
      return 0;
   }

   x =           data[1] | (data[2]<<7);
   y = 0x3FFF - (data[3] | (data[4]<<7));

   if (avgx < 0) { /* press event */
      GET_TIME(tv);
      if (DIF_TIME(uptv, tv) < opt_time) {
         /* count as button press placed at finger-up pixel */
         state->buttons = GPM_B_LEFT;
         realposx = avgx = upx; state->x = REAL_TO_XCELL(realposx);
         realposy = avgy = upy; state->y = REAL_TO_YCELL(realposy);
         upx = (upx - x); /* upx and upy become offsets to use for this drag */
         upy = (upy - y);
         return 0;
      }
      /* else, count as a new motion event */
      tv.tv_sec = 0; /* invalidate */
      realposx = avgx = x; state->x = REAL_TO_XCELL(realposx);
      realposy = avgy = y; state->y = REAL_TO_YCELL(realposy);
   }

   state->buttons = 0;
   if (tv.tv_sec) { /* a drag event: use position relative to press */
      x += upx;
      y += upy;
      state->buttons = GPM_B_LEFT;
   }

   realposx = avgx = (9*avgx + x)/10; state->x = REAL_TO_XCELL(realposx);
   realposy = avgy = (9*avgy + y)/10; state->y = REAL_TO_YCELL(realposy);

   return 0;

   #undef REAL_TO_XCELL
   #undef REAL_TO_YCELL
   #undef GET_TIME
   #undef DIF_TIME
}

/*
 * This decoder is copied and adapted from the above mtouch.
 * However, this one uses argv, which should be ported above as well.
 * These variables are used for option passing
 */
static int gunze_avg = 9; /* the bigger the smoother */
static int gunze_calib[4]; /* x0,y0 x1,y1 (measured at 1/8 and 7/8) */
static int gunze_debounce = 100; /* milliseconds: ignore shorter taps */

static int M_gunze(Gpm_Event *state,  unsigned char *data)
{
   /*
    * This generates button-1 events, by now.
    * Check README.gunze for additional information.
    */
   int x, y;
   static int avgx, avgy;          /* average over time, for smooth feeling */
   static int upx, upy;            /* keep track of last finger-up place */
   static int released = 0, dragging = 0;
   static struct timeval uptv, tv; /* time of last up, and down events */
   int timediff;
    
   #define REAL_TO_XCELL(x) (x * win.ws_col / 0x3FFF)
   #define REAL_TO_YCELL(y) (y * win.ws_row / 0x3FFF)
    
   #define GET_TIME(tv) (gettimeofday(&tv, (struct timezone *)NULL))
   #define DIF_TIME(t1,t2) ((t2.tv_sec -t1.tv_sec) *1000+ \
                           (t2.tv_usec-t1.tv_usec)/1000)

   if (data[0] == 'R') {
      /*
       * finger-up event: this is usually offset a few pixels,
       * so ignore this x and y values. And invalidate avg.
       */
      upx = avgx;
      upy = avgy;
      GET_TIME(uptv); /* ready for the next finger-down */
      state->buttons = 0;
      released=1;
      return 0;
   }

   if(sscanf(data+1, "%d,%d", &x, &y)!= 2) {
      gpm_report(GPM_PR_INFO,GPM_MESS_GUNZE_INV_PACK,data);
      return 10; /* eat one byte only, leave ten of them */
   }
    /*
     * First of all, calibrate decoded data;
     * the four numbers are 1/8X 1/8Y, 7/8X, 7/8Y
     */
   x = 128 + 768 * (x - gunze_calib[0])/(gunze_calib[2]-gunze_calib[0]);
   y = 128 + 768 * (y - gunze_calib[1])/(gunze_calib[3]-gunze_calib[1]);

   /* range is 0-1023, rescale it upwards (0-REALPOS_MAX) */
   x = x * REALPOS_MAX / 1023;
   y = REALPOS_MAX - y * REALPOS_MAX / 1023;

   if (x<0) x = 0; if (x > REALPOS_MAX) x = REALPOS_MAX;
   if (y<0) y = 0; if (y > REALPOS_MAX) y = REALPOS_MAX;

    /*
     * there are some issues wrt press events: since the device needs a
     * non-negligible pressure to detect stuff, it sometimes reports
     * quick up-down sequences, that actually are just the result of a
     * little bouncing of the finger. Therefore, we should discard any
     * release-press pair. This can be accomplished at press time.
     */

   if (released) { /* press event -- or just bounce*/
      GET_TIME(tv);
      timediff = DIF_TIME(uptv, tv);
      released = 0;
      if (timediff > gunze_debounce && timediff < opt_time) {
         /* count as button press placed at finger-up pixel */
         dragging = 1;
         state->buttons = GPM_B_LEFT;
         realposx = avgx = upx; state->x = REAL_TO_XCELL(realposx);
         realposy = avgy = upy; state->y = REAL_TO_YCELL(realposy);
         upx = (upx - x); /* upx-upy become offsets to use for this drag */
         upy = (upy - y);
         return 0;
      }
      if (timediff <= gunze_debounce) {
         /* just a bounce, invalidate offset, leave dragging alone */
         upx = upy = 0;
      } else {
         /* else, count as a new motion event, reset avg */
         dragging = 0;
         realposx = avgx = x;
         realposy = avgy = y;
      }
   }

   state->buttons = 0;
   if (dragging) { /* a drag event: use position relative to press */
      x += upx;
      y += upy;
      state->buttons = GPM_B_LEFT;
   }

   realposx = avgx = (gunze_avg * avgx + x)/(gunze_avg+1);
   state->x = REAL_TO_XCELL(realposx);
   realposy = avgy = (gunze_avg * avgy + y)/(gunze_avg+1);
   state->y = REAL_TO_YCELL(realposy);

   return 0;

   #undef REAL_TO_XCELL
   #undef REAL_TO_YCELL
   #undef GET_TIME
   #undef DIF_TIME
}

/* Support for DEC VSXXX-AA and VSXXX-GA serial mice used on   */
/* DECstation 5000/xxx, DEC 3000 AXP and VAXstation 4000       */
/* workstations                                                */
/* written 2001/07/11 by Karsten Merker (merker@linuxtag.org)  */
/* modified (completion of the protocol specification and      */
/* corresponding correction of the protocol identification     */
/* mask) 2001/07/12 by Maciej W. Rozycki (macro@ds2.pg.gda.pl) */

static int M_vsxxx_aa(Gpm_Event *state, unsigned char *data)
{

/* The mouse protocol is as follows:
 * 4800 bits per second, 8 data bits, 1 stop bit, odd parity
 * 3 data bytes per data packet:
 *              7     6     5     4     3     2     1     0
 * First Byte:  1     0     0   SignX SignY  LMB   MMB   RMB
 * Second Byte  0     DX    DX    DX    DX    DX    DX    DX
 * Third Byte   0     DY    DY    DY    DY    DY    DY    DY
 *
 * SignX:     sign bit for X-movement
 * SignY:     sign bit for Y-movement
 * DX and DY: 7-bit-absolute values for delta-X and delta-Y, sign extensions
 *            are in SignX resp. SignY.
 *
 * There are a few commands the mouse accepts:
 * "D" selects the prompt mode,
 * "P" requests a mouse's position (also selects the prompt mode),
 * "R" selects the incremental stream mode,
 * "T" performs a self test and identification (power-up-like),
 * "Z" performs undocumented test functions (a byte follows).
 * Parity as well as bit #7 of commands are ignored by the mouse.
 *
 * 4 data bytes per self test packet (useful for hot-plug):
 *              7     6     5     4     3     2     1     0
 * First Byte:  1     0     1     0     R3    R2    R1    R0
 * Second Byte  0     M2    M1    M0    0     0     1     0
 * Third Byte   0     E6    E5    E4    E3    E2    E1    E0
 * Fourth Byte  0     0     0     0     0    LMB   MMB   RMB
 *
 * R3-R0:     revision,
 * M2-M0:     manufacturer location code,
 * E6-E0:     error code:
 *            0x00-0x1f: no error (fourth byte is button state),
 *            0x3d:      button error (fourth byte specifies which),
 *            else:      other error.
 * 
 * The mouse powers up in the prompt mode but we use the stream mode.
 */

  state->buttons = data[0]&0x07;
  state->dx = (data[0]&0x10) ? data[1] : -data[1];
  state->dy = (data[0]&0x08) ? -data[2] : data[2];
  return 0;
}

/*  Genius Wizardpad tablet  --  Matt Kimball (mkimball@xmission.com)  */
static int wizardpad_width = -1;
static int wizardpad_height = -1;
static int M_wp(Gpm_Event *state,  unsigned char *data)
{
   int x, y, pressure;

   x = ((data[4] & 0x1f) << 12) | ((data[3] & 0x3f) << 6) | (data[2] & 0x3f);
   state->x = x * win.ws_col / (wizardpad_width * 40);
   realposx = x * 16383 / (wizardpad_width * 40);

   y = ((data[7] & 0x1f) << 12) | ((data[6] & 0x3f) << 6) | (data[5] & 0x3f);
   state->y = win.ws_row - y * win.ws_row / (wizardpad_height * 40) - 1;
   realposy = 16383 - y * 16383 / (wizardpad_height * 40) - 1;  

   pressure = ((data[9] & 0x0f) << 4) | (data[8] & 0x0f);

   state->buttons=
      (pressure >= 0x20) * GPM_B_LEFT +
      !!(data[1] & 0x02) * GPM_B_RIGHT +
      /* the 0x08 bit seems to catch either of the extra buttons... */
      !!(data[1] & 0x08) * GPM_B_MIDDLE;

   return 0;
}

/*========================================================================*/
/* Then, mice should be initialized */

static Gpm_Type* I_empty(int fd, unsigned short flags,
    struct Gpm_Type *type, int argc, char **argv)
{
    if (check_no_argv(argc, argv)) return NULL;
    return type;
}

static int setspeed(int fd,int old,int new,int needtowrite,unsigned short flags)
{
   struct termios tty;
   char *c;

   tcgetattr(fd, &tty);
  
   tty.c_iflag = IGNBRK | IGNPAR;
   tty.c_oflag = 0;
   tty.c_lflag = 0;
   tty.c_line = 0;
   tty.c_cc[VTIME] = 0;
   tty.c_cc[VMIN] = 1;

   switch (old) {
      case 19200: tty.c_cflag = flags | B19200; break;
      case  9600: tty.c_cflag = flags |  B9600; break;
      case  4800: tty.c_cflag = flags |  B4800; break;
      case  2400: tty.c_cflag = flags |  B2400; break;
      case  1200:
      default:    tty.c_cflag = flags | B1200; break;
   }

   tcsetattr(fd, TCSAFLUSH, &tty);

   switch (new) {
      case 19200: c = "*r";  tty.c_cflag = flags | B19200; break;
      case  9600: c = "*q";  tty.c_cflag = flags |  B9600; break;
      case  4800: c = "*p";  tty.c_cflag = flags |  B4800; break;
      case  2400: c = "*o";  tty.c_cflag = flags |  B2400; break;
      case  1200:
      default:    c = "*n";  tty.c_cflag = flags | B1200; break;
   }

   if (needtowrite) write(fd, c, 2);
   usleep(100000);
   tcsetattr(fd, TCSAFLUSH, &tty);
   return 0;
}



static struct {
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

static Gpm_Type* I_serial(int fd, unsigned short flags,
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
      setspeed(fd, i, opt_baud, (type->fun != M_mman) /* write */, flags);

   /*
    * reset the MouseMan/TrackMan to use the 3/4 byte protocol
    * (Stephen Lee, sl14@crux1.cit.cornell.edu)
    * Changed after 1.14; why not having "I_mman" now?
    */
   if (type->fun==M_mman) {
      setspeed(fd, 1200, 1200, 0, flags); /* no write */
      write(fd, "*X", 2);
      setspeed(fd, 1200, opt_baud, 0, flags); /* no write */
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

static Gpm_Type* I_logi(int fd, unsigned short flags,
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
   for (i=9600; i>=1200; i/=2) setspeed(fd, i, opt_baud, 1 /* write */, flags);

   /* this stuff is peculiar of logitech mice, also for the serial ones */
   write(fd, "S", 1);
   setspeed(fd, opt_baud, opt_baud, 1 /* write */,
      CS8 |PARENB |PARODD |CREAD |CLOCAL |HUPCL);

   /* configure the sample rate */
   for (i=0;opt_sample<=sampletab[i].sample;i++) ;
   write(fd,sampletab[i].code,1);
   return type;
}

static Gpm_Type *I_wacom(int fd, unsigned short flags,
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

static Gpm_Type *I_pnp(int fd, unsigned short flags,
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
static int read_mouse_id(int fd)
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

/**
 * Writes the given data to the ps2-type mouse.
 * Checks for an ACK from each byte.
 * 
 * Returns 0 if OK, or >0 if 1 or more errors occurred.
 */
static int write_to_mouse(int fd, unsigned char *data, size_t len)
{
   int i;
   int error = 0;
   for (i = 0; i < len; i++) {
      unsigned char c;
      write(fd, &data[i], 1);
      read(fd, &c, 1);
      if (c != GPM_AUX_ACK) error++;
   }

   /* flush any left-over input */
   usleep (30000);
   tcflush (fd, TCIFLUSH);
   return(error);
}


/* intellimouse, ps2 version: Ben Pfaff and Colin Plumb */
/* Autodetect: Steve Bennett */
static Gpm_Type *I_imps2(int fd, unsigned short flags, struct Gpm_Type *type,
                                                       int argc, char **argv)
{
   int id;
   static unsigned char basic_init[] = { GPM_AUX_ENABLE_DEV, GPM_AUX_SET_SAMPLE, 100 };
   static unsigned char imps2_init[] = { GPM_AUX_SET_SAMPLE, 200, GPM_AUX_SET_SAMPLE, 100, GPM_AUX_SET_SAMPLE, 80, };
   static unsigned char ps2_init[] = { GPM_AUX_SET_SCALE11, GPM_AUX_ENABLE_DEV, GPM_AUX_SET_SAMPLE, 100, GPM_AUX_SET_RES, 3, };

   /* Do a basic init in case the mouse is confused */
   write_to_mouse(fd, basic_init, sizeof (basic_init));

   /* Now try again and make sure we have a PS/2 mouse */
   if (write_to_mouse(fd, basic_init, sizeof (basic_init)) != 0) {
      gpm_report(GPM_PR_ERR,GPM_MESS_IMPS2_INIT);
      return(NULL);
   }

   /* Try to switch to 3 button mode */
   if (write_to_mouse(fd, imps2_init, sizeof (imps2_init)) != 0) {
      gpm_report(GPM_PR_ERR,GPM_MESS_IMPS2_FAILED);
      return(NULL);
   }

   /* Read the mouse id */
   id = read_mouse_id(fd);
   if (id == GPM_AUX_ID_ERROR) {
      gpm_report(GPM_PR_ERR,GPM_MESS_IMPS2_MID_FAIL);
      id = GPM_AUX_ID_PS2;
   }

   /* And do the real initialisation */
   if (write_to_mouse(fd, ps2_init, sizeof (ps2_init)) != 0) {
      gpm_report(GPM_PR_ERR,GPM_MESS_IMPS2_SETUP_FAIL);
   }

   if (id == GPM_AUX_ID_IMPS2) {
   /* Really an intellipoint, so initialise 3 button mode (4 byte packets) */
      gpm_report(GPM_PR_INFO,GPM_MESS_IMPS2_AUTO);
      return type;
   }
   if (id != GPM_AUX_ID_PS2) {
      gpm_report(GPM_PR_ERR,GPM_MESS_IMPS2_BAD_ID, id);
   }
   else gpm_report(GPM_PR_INFO,GPM_MESS_IMPS2_PS2);

   for (type=mice; type->fun; type++)
      if (strcmp(type->name, "ps2") == 0) return(type);

   /* ps2 was not found!!! */
   return(NULL);
}

/*
 * This works with Dexxa Optical Mouse, but because in X same initstring
 * is named ExplorerPS/2 so I named it in the same way.
 */
static Gpm_Type *I_exps2(int fd, unsigned short flags,
          struct Gpm_Type *type, int argc, char **argv)
{
   static unsigned char s1[] = { 243, 200, 243, 200, 243, 80, };

   if (check_no_argv(argc, argv)) return NULL;

   write (fd, s1, sizeof (s1));
   usleep (30000);
   tcflush (fd, TCIFLUSH);
   return type;
}

static Gpm_Type *I_twid(int fd, unsigned short flags,
         struct Gpm_Type *type, int argc, char **argv)
{

   if (check_no_argv(argc, argv)) return NULL;

   if (twiddler_key_init() != 0) return NULL;
   /*
   * the twiddler is a serial mouse: just drop dtr
   * and run at 2400 (unless specified differently) 
   */
   if(opt_baud==DEF_BAUD) opt_baud = 2400;
   argv[1] = "dtr"; /* argv[1] is guaranteed to be NULL (this is dirty) */
   return I_serial(fd, flags, type, argc, argv);
}

static Gpm_Type *I_calus(int fd, unsigned short flags,
          struct Gpm_Type *type, int argc, char **argv)
{
   if (check_no_argv(argc, argv)) return NULL;

   if (opt_baud == 1200) opt_baud=9600; /* default to 9600 */
   return I_serial(fd, flags, type, argc, argv);
}

/* synaptics touchpad, ps2 version: Henry Davies */
static Gpm_Type *I_synps2(int fd, unsigned short flags,
           struct Gpm_Type *type, int argc, char **argv)
{
   syn_ps2_init (fd);
   return type;
}


static Gpm_Type *I_summa(int fd, unsigned short flags,
          struct Gpm_Type *type, int argc, char **argv) 
{
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

static Gpm_Type *I_mtouch(int fd, unsigned short flags,
           struct Gpm_Type *type, int argc, char **argv)
{
   struct termios tty;

   /* Set speed to 9600bps (copied from I_summa, above :) */
   tcgetattr(fd, &tty);
   tty.c_iflag = IGNBRK | IGNPAR;
   tty.c_oflag = 0;
   tty.c_lflag = 0;
   tty.c_line = 0;
   tty.c_cc[VTIME] = 0;
   tty.c_cc[VMIN] = 1;
   tty.c_cflag = B9600|CS8|CREAD|CLOCAL|HUPCL;
   tcsetattr(fd, TCSAFLUSH, &tty);


   /* Turn it to "format tablet" and "mode stream" */
   write(fd,"\001MS\r\n\001FT\r\n",10);
  
   return type;
}

/* simple initialization for the gunze touchscreen */
static Gpm_Type *I_gunze(int fd, unsigned short flags,
           struct Gpm_Type *type, int argc, char **argv)
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
   if (opt_baud == DEF_BAUD) opt_baud = 19200; /* force 19200 as default */
   if (opt_baud != 9600 && opt_baud != 19200) {
      gpm_report(GPM_PR_ERR,GPM_MESS_GUNZE_WRONG_BAUD,option.progname, argv[0]);
      opt_baud = 19200;
   }
   tcgetattr(fd, &tty);
   tty.c_iflag = IGNBRK | IGNPAR;
   tty.c_oflag = 0;
   tty.c_lflag = 0;
   tty.c_line = 0;
   tty.c_cc[VTIME] = 0;
   tty.c_cc[VMIN] = 1;
   tty.c_cflag = (opt_baud == 9600 ? B9600 : B19200) |CS8|CREAD|CLOCAL|HUPCL;
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
      if (gunze_calib[1] == gunze_calib[4]) calibok = 0;
      fclose(f);
   }
   if (!calibok) {
      gpm_report(GPM_PR_ERR,GPM_MESS_GUNZE_CALIBRATE, option.progname);
      gunze_calib[0] = gunze_calib[1] = 128; /* 1/8 */
      gunze_calib[2] = gunze_calib[3] = 896; /* 7/8 */
   }
   return type;
}

/*  Genius Wizardpad tablet  --  Matt Kimball (mkimball@xmission.com)  */
static Gpm_Type *I_wp(int fd, unsigned short flags,
            struct Gpm_Type *type, int argc, char **argv)
{
   struct termios tty;
   char tablet_info[256];
   int count, pos, size;

   /* Set speed to 9600bps (copied from I_summa, above :) */
   tcgetattr(fd, &tty);
   tty.c_iflag = IGNBRK | IGNPAR;
   tty.c_oflag = 0;
   tty.c_lflag = 0;
   tty.c_line = 0;
   tty.c_cc[VTIME] = 0;
   tty.c_cc[VMIN] = 1;
   tty.c_cflag = B9600|CS8|CREAD|CLOCAL|HUPCL;
   tcsetattr(fd, TCSAFLUSH, &tty);

   /*  Reset the tablet (':') and put it in remote mode ('S') so that
     it isn't sending anything to us.  */
   write(fd, ":S", 2);
   tcsetattr(fd, TCSAFLUSH, &tty);

   /*  Query the model of the tablet  */
   write(fd, "T", 1);
   sleep(1);
   count = read(fd, tablet_info, 255);

   /*  The tablet information should start with "KW" followed by the rest of
     the model number.  If it isn't there, it probably isn't a WizardPad.  */
   if(count < 2) return NULL;
   if(tablet_info[0] != 'K' || tablet_info[1] != 'W') return NULL;

   /*  Now, we want the width and height of the tablet.  They should be 
     of the form "X###" and "Y###" where ### is the number of units of
     the tablet.  The model I've got is "X130" and "Y095", but I guess
     there might be other ones sometime.  */
   for(pos = 0; pos < count; pos++) {
      if(tablet_info[pos] == 'X' || tablet_info[pos] == 'Y') {
         if(pos + 3 < count) {
            size = (tablet_info[pos + 1] - '0') * 100 +
                   (tablet_info[pos + 2] - '0') * 10 +
                   (tablet_info[pos + 3] - '0');
            if(tablet_info[pos] == 'X') {
               wizardpad_width = size;
            } else {
               wizardpad_height = size;
            }
         }
      }
   }

   /*  Set the tablet to stream mode with 180 updates per sec.  ('O')  */
   write(fd, "O", 1);

   return type;
}

/*========================================================================*/
/* Finally, the table */
#define STD_FLG (CREAD|CLOCAL|HUPCL)

/*
 * Note that mman must be the first, and ms the second (I use this info
 * in mouse-test.c, as a quick and dirty hack
 *
 * We should clean up mouse-test and sort the table alphabeticly! --nico
 */

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
#ifdef HAVE_LINUX_INPUT_H
   {"evdev", "Linux Event Device",
            "", M_evdev, I_empty, STD_FLG,
                        {0x00, 0x00, 0x00, 0x00} , 16, 16, 0, 0, NULL},
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
                                {0xf8, 0x80, 0x00, 0x00}, 3, 1, 0, 0, 0},
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
