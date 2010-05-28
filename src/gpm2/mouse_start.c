/*
 *   Copyright (c) 2010 Nico Schottelius <nico-gpm2 () schottelius.org>
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
 *
 * Start a mouse driver
 *
 */

#include <unistd.h>
#include "gpm2-daemon.h"

int mouse_start(struct gpm2_mouse *mouse)
{

   /* create communication channel */
   if(pipe(mouse->pipe) == -1) {
      perror(mouse->name);
      return 0;
   }

   /* fork away */
   mouse->pid = fork();
   if(mouse->pid == -1) {
      perror(mouse->name);
      return 0;
   }

   /* return in parent */
   if(mouse->pid != 0) return 1;

   /* open device */
   mouse->fd = open(mouse->name, 0);
   if(mouse->fd == -1) {
      perror(mouse->name);
      return 0;
   }

   /* connect device to stdin/out */
   if(dup2(mouse->fd, STDIN_FILENO) == -1) {
      perror(mouse->name);
      return 0;
   }

   if(dup2(mouse->fd, STDOUT_FILENO) == -1) {
      perror(mouse->name);
      return 0;
   }

   /* drop priviliges (if configured) */
   /* launch protocol handler */



}
