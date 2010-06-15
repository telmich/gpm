/*
 * disable-paste.c - trivial program to stop gpm from pasting the
 * current selection until the next user action
 *
 * Copyright (C) 1998 Ian Zimmerman <itz@rahul.net>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include "headers/gpm.h"
#include "headers/gpmInt.h"

int 
main(int argc, char** argv)
{
  Gpm_Connect conn;
  const int len = sizeof(Gpm_Connect);
  int exit_status = 0;
  conn.eventMask = (unsigned short)(-1); conn.defaultMask = 0;
  conn.minMod = 0; conn.maxMod = (unsigned short)(-1);
  
  if (0 > Gpm_Open(&conn,0)) {
    fprintf(stderr,"disable-paste: cannot open mouse connection\n");
    exit(1);
  }
  conn.vc = GPM_REQ_NOPASTE;
  conn.pid = 0;
  if (len > write(gpm_fd, &conn, len)) {
    fprintf(stderr,"disable-paste: cannot write request\n");
    exit_status = 2;
  }
  Gpm_Close();
  exit(exit_status);
}
