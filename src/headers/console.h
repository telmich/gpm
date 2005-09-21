/*
 * console.h - GPM console and selection/paste handling
 *
 * Copyright (C) 2003  Dmitry Torokhov <dtor@mail.ru>
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

#ifndef __GPM_CONSOLE_H_
#define __GPM_CONSOLE_H_

struct gpm_console {
   char  *device;
   char  *charset;
   int   max_x, max_y;
};

extern struct gpm_console console;

int   open_console(int mode);
char  *get_console_name();
char  *compose_vc_name(int vc);
int   is_text_console(void);
void  wait_text_console(void);
void  refresh_console_size(void);
int   is_console_owner(int vc, uid_t uid);
int   get_console_state(unsigned char *shift_state);
void  console_load_lut(void);

#endif /* __GPM_CONSOLE_H_ */
