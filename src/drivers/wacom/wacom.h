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

#ifndef _GPM_WACOM_H
#define _GPM_WACOM_H

struct WC_MODELL {
   char name[15];
   char magic[3];
   int  maxX;
   int  maxY;
   int  border;
   int  treshold;
};

extern int  WacomModell;
extern int  WacomAbsoluteWanted;
extern int  wmaxx;
extern int  wmaxy;
extern char upmbuf[25];

extern struct WC_MODELL wcmodell[3];

#define IsA(m) ((WacomModell==(-1))? 0:!strcmp(#m,wcmodell[WacomModell].name))


#endif
