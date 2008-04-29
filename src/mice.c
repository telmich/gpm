
/*
 * mice.c - table of mice for gpm
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
 *
 ********/

#include <termios.h>            /* for FLAGS */

#include "mice.h"
#include "drivers.h"

 /*
  * For those who are trying to add a new type, here a brief
  * description of the structure. Please refer to mice.h and drivers/
  * for more information:
  *
  * The first three strings are the name, an help line, a long name (if any)
  * Then come the functions: the decoder and the initializazion function
  *     (called I_* and M_*)
  *
  * Follows an array of four bytes: it is the protocol-identification, based
  *     on the first two bytes of a packet: if
  *     "((byte0 & proto[0]) == proto[1]) && ((byte1 & proto[2]) == proto[3])"
  *     then we are at the beginning of a packet.
  *
  * The following numbers are:
  *     bytes-per-packet,
  *     bytes-to-read (use 1, bus mice are pathological)
  *     has-extra-byte (boolean, for mman pathological protocol)
  *     is-absolute-coordinates (boolean)
  *
  * Finally, a pointer to a repeater function, if any.
  *
  */

Gpm_Type mice[] = {
   {"mman", "The \"MouseMan\" and similar devices (3/4 bytes per packet).",
    "Mouseman", M_mman, I_serial, CS7 | STD_FLG,        /* first */
    {0x40, 0x40, 0x40, 0x00}
    , 3, 1, 1, 0, 0}
   ,
   {"ms", "The original ms protocol, with a middle-button extension.",
    "", M_ms, I_serial, CS7 | STD_FLG,
    {0x40, 0x40, 0x40, 0x00}
    , 3, 1, 0, 0, 0}
   ,
   {"acecad", "Acecad tablet absolute mode(Sumagrapics MM-Series mode)",
    "", M_summa, I_summa, STD_FLG,
    {0x80, 0x80, 0x00, 0x00}
    , 7, 1, 0, 1, 0}
   ,
   {"bare", "Unadorned ms protocol. Needed with some 2-buttons mice.",
    "Microsoft", M_bare, I_serial, CS7 | STD_FLG,
    {0x40, 0x40, 0x40, 0x00}
    , 3, 1, 0, 0, 0}
   ,
   {"bm", "Micro$oft busmice and compatible devices.",
    "BusMouse", M_bm, I_empty, STD_FLG, /* bm is sun */
    {0xf8, 0x80, 0x00, 0x00}
    , 3, 3, 0, 0, 0}
   ,
   {"brw", "Fellowes Browser - 4 buttons (and a wheel) (dual protocol?)",
    "", M_brw, I_pnp, CS7 | STD_FLG,
    {0xc0, 0x40, 0xc0, 0x00}
    , 4, 1, 0, 0, 0}
   ,
   {"cal", "Calcomp UltraSlate",
    "", M_calus, I_calus, CS8 | CSTOPB | STD_FLG,
    {0x80, 0x80, 0x80, 0x00}
    , 6, 6, 0, 1, 0}
   ,
   {"calr", "Calcomp UltraSlate - relative mode",
    "", M_calus_rel, I_calus, CS8 | CSTOPB | STD_FLG,
    {0x80, 0x80, 0x80, 0x00}
    , 6, 6, 0, 0, 0}
   ,
   {"etouch", "EloTouch touch-screens (only button-1 events, by now)",
    "", M_etouch, I_etouch, STD_FLG,
    {0xFF, 0x55, 0xFF, 0x54}
    , 7, 1, 0, 1, NULL}
   ,
#ifdef HAVE_LINUX_INPUT_H
   {"evdev", "Linux Event Device",
    "", M_evdev, I_empty, STD_FLG,
    {0x00, 0x00, 0x00, 0x00}
    , 16, 16, 0, 0, NULL}
   ,
   {"evabs", "Linux Event Device - absolute mode",
    "", M_evabs, I_empty, STD_FLG,
    {0x00, 0x00, 0x00, 0x00}
    , 16, 16, 0, 1, NULL}
   ,
#endif                          /* HAVE_LINUX_INPUT_H */
   {"exps2", "IntelliMouse Explorer (ps2) - 3 buttons, wheel unused",
    "ExplorerPS/2", M_imps2, I_exps2, STD_FLG,
    {0xc0, 0x00, 0x00, 0x00}
    , 4, 1, 0, 0, 0}
   ,
#ifdef HAVE_LINUX_JOYSTICK_H
   {"js", "Joystick mouse emulation",
    "Joystick", M_js, NULL, 0,
    {0xFC, 0x00, 0x00, 0x00}
    , 12, 12, 0, 0, 0}
   ,
#endif
   {"genitizer", "\"Genitizer\" tablet, in relative mode.",
    "", M_geni, I_serial, CS8 | PARENB | PARODD,
    {0x80, 0x80, 0x00, 0x00}
    , 3, 1, 0, 0, 0}
   ,
   {"gunze", "Gunze touch-screens (only button-1 events, by now)",
    "", M_gunze, I_gunze, STD_FLG,
    {0xF9, 0x50, 0xF0, 0x30}
    , 11, 1, 0, 1, NULL}
   ,
   {"imps2", "Microsoft Intellimouse (ps2)-autodetect 2/3 buttons,wheel unused",
    "", M_imps2, I_imps2, STD_FLG,
    {0xC0, 0x00, 0x00, 0x00}
    , 4, 1, 0, 0, R_imps2}
   ,
   {"logi", "Used in some Logitech devices (only serial).",
    "Logitech", M_logi, I_logi, CS8 | CSTOPB | STD_FLG,
    {0xe0, 0x80, 0x80, 0x00}
    , 3, 3, 0, 0, 0}
   ,
   {"logim", "Turn logitech into Mouse-Systems-Compatible.",
    "", M_logimsc, I_serial, CS8 | CSTOPB | STD_FLG,
    {0xf8, 0x80, 0x00, 0x00}
    , 5, 1, 0, 0, 0}
   ,
   {"mm", "MM series. Probably an old protocol...",
    "MMSeries", M_mm, I_serial, CS8 | PARENB | PARODD | STD_FLG,
    {0xe0, 0x80, 0x80, 0x00}
    , 3, 1, 0, 0, 0}
   ,
   {"ms3", "Microsoft Intellimouse (serial) - 3 buttons, wheel unused",
    "", M_ms3, I_pnp, CS7 | STD_FLG,
    {0xc0, 0x40, 0xc0, 0x00}
    , 4, 1, 0, 0, R_ms3}
   ,
   {"ms+", "Like 'ms', but allows dragging with the middle button.",
    "", M_ms_plus, I_serial, CS7 | STD_FLG,
    {0x40, 0x40, 0x40, 0x00}
    , 3, 1, 0, 0, 0}
   ,
   {"ms+lr", "'ms+', but you can reset m by pressing lr (see man page).",
    "", M_ms_plus_lr, I_serial, CS7 | STD_FLG,
    {0x40, 0x40, 0x40, 0x00}
    , 3, 1, 0, 0, 0}
   ,
   {"msc", "Mouse-Systems-Compatible (5bytes). Most 3-button mice.",
    "MouseSystems", M_msc, I_serial, CS8 | CSTOPB | STD_FLG,
    {0xf8, 0x80, 0x00, 0x00}
    , 5, 1, 0, 0, R_msc}
   ,
   {"mtouch", "MicroTouch touch-screens (only button-1 events, by now)",
    "", M_mtouch, I_mtouch, STD_FLG,
    {0x80, 0x80, 0x80, 0x00}
    , 5, 1, 0, 1, NULL}
   ,
   {"ncr", "Ncr3125pen, found on some laptops",
    "", M_ncr, NULL, STD_FLG,
    {0x08, 0x08, 0x00, 0x00}
    , 7, 7, 0, 1, 0}
   ,
   {"netmouse", "Genius NetMouse (ps2) - 2 buttons and 2 buttons 'up'/'down'.",
    "", M_netmouse, I_netmouse, CS7 | STD_FLG,
    {0xc0, 0x00, 0x00, 0x00}
    , 4, 1, 0, 0, 0}
   ,
   {"pnp", "Plug and pray. New mice may not run with '-t ms'.",
    "", M_bare, I_pnp, CS7 | STD_FLG,
    {0x40, 0x40, 0x40, 0x00}
    , 3, 1, 0, 0, 0}
   ,
   {"ps2", "Busmice of the ps/2 series. Most busmice, actually.",
    "PS/2", M_ps2, I_ps2, STD_FLG,
    {0xc0, 0x00, 0x00, 0x00}
    , 3, 1, 0, 0, R_ps2}
   ,
   {"sun", "'msc' protocol, but only 3 bytes per packet.",
    "", M_sun, I_serial, CS8 | CSTOPB | STD_FLG,
    {0xf8, 0x80, 0x00, 0x00}
    , 3, 1, 0, 0, R_sun}
   ,
   {"summa", "Summagraphics or Genius tablet absolute mode(MM-Series)",
    "", M_summa, I_summa, STD_FLG,
    {0x80, 0x80, 0x00, 0x00}
    , 5, 1, 0, 1, R_summa}
   ,
   {"syn", "The \"Synaptics\" serial TouchPad.",
    "synaptics", M_synaptics_serial, I_serial, CS7 | STD_FLG,
    {0x40, 0x40, 0x40, 0x00}
    , 6, 6, 1, 0, 0}
   ,
   {"synps2", "The \"Synaptics\" PS/2 TouchPad",
    "synaptics_ps2", M_synaptics_ps2, I_synps2, STD_FLG,
    {0x80, 0x80, 0x00, 0x00}
    , 6, 1, 1, 0, 0}
   ,
   {"twid", "Twidddler keyboard",
    "", M_twid, I_twid, CS8 | STD_FLG,
    {0x80, 0x00, 0x80, 0x80}
    , 5, 1, 0, 0, 0}
   ,
   {"vsxxxaa", "The DEC VSXXX-AA/GA serial mouse on DEC workstations.",
    "", M_vsxxx_aa, I_serial, CS8 | PARENB | PARODD | STD_FLG,
    {0xe0, 0x80, 0x80, 0x00}
    , 3, 1, 0, 0, 0}
   ,
   {"wacom", "Wacom Protocol IV Tablets: Pen+Mouse, relative+absolute mode",
    "", M_wacom, I_wacom, STD_FLG,
    {0x80, 0x80, 0x80, 0x00}
    , 7, 1, 0, 0, 0}
   ,
   {"wp", "Genius WizardPad tablet",
    "wizardpad", M_wp, I_wp, STD_FLG,
    {0xFA, 0x42, 0x00, 0x00}
    , 10, 1, 0, 1, 0}
   ,
   {"", "",
    "", NULL, NULL, 0,
    {0x00, 0x00, 0x00, 0x00}
    , 0, 0, 0, 0, 0}
};
