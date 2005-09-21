/*
 * client.h - GPM client handling (server side)
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

#ifndef __GPM_CLIENT_H
#define __GPM_CLIENT_H_

#ifdef HAVE_LINUX_TTY_H
#include <linux/tty.h>
#endif

#include "headers/gpm.h"

/* FIXME: still needed ?? */
/* How many virtual consoles are managed? */
#ifndef MAX_NR_CONSOLES
#  define MAX_NR_CONSOLES 64 /* this is always sure */
#endif

#define MAX_VC    MAX_NR_CONSOLES  /* doesn't work before 1.3.77 */

struct client_info {
  Gpm_Connect data;
  int fd;
  struct client_info *next;
};

struct Gpm_Event;

extern struct client_info *cinfo[MAX_VC + 1];

int   listen_for_clients(void);
struct client_info *accept_client_connection(int fd);
void  remove_client(struct client_info *ci, int vc);
void  notify_clients_resize(void);
int   do_client(struct client_info *cinfo, struct Gpm_Event *event);
int   process_client_request(struct client_info *ci, int vc,
                             int x, int y, int buttons, int clicks,
                             int three_button_mouse);

#endif /* __GPM_CLIENT_H_ */
