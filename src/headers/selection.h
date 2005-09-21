/*
 * selection.h - GPM selection/copy/paste handling
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

#ifndef __GPM_SELECTION_H_
#define __GPM_SELECTION_H_

struct sel_options {
   int      aged;
   int      age_limit;
   int      ptrdrag;
};

struct Gpm_Event;

extern struct sel_options sel_opts; /* only one exists */

void  do_selection(struct Gpm_Event *event, int three_button_mode);
void  selection_disable_paste(void);

#endif /* __GPM_SELECTION_H_ */
