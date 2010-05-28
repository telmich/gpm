/*
 *   Copyright (c) 2010    Nico Schottelius <nico-gpm2 () schottelius.org>
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
 *   Internal stuff
 */

#ifndef _GPM2_DAEMON_H
#define _GPM2_DAEMON_H

#include <gpm2.h>

int init();
int mouse_add(char *name, char *proto);
int mouse_init();
int mouse_start(struct gpm2_mouse *);

extern struct gpm2_mouse mice;

#endif
