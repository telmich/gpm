/*
 * report-lib.c: the exported version of gpm_report. used in Gpm_Open and co.
 *
 * Copyright (c) 2001        Nico Schottelius <nico@schottelius.org>
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
 */

#include <stdio.h>      /* NULL */
#include <stdarg.h>     /* va_arg/start/... */
#include <stdlib.h>     /* exit() */

#include "headers/message.h"

void gpm_report(int line, char *file, int stat, char *text, ... )
{
   char *string = NULL;
   va_list ap;
   va_start(ap,text);

   switch(stat) {
      case GPM_STAT_INFO : string = GPM_TEXT_INFO ; break;
      case GPM_STAT_WARN : string = GPM_TEXT_WARN ; break;
      case GPM_STAT_ERR  : string = GPM_TEXT_ERR  ; break;
      case GPM_STAT_DEBUG: string = GPM_TEXT_DEBUG; break;
      case GPM_STAT_OOPS : string = GPM_TEXT_OOPS; break;
   }
   fprintf(stderr,"%s[%s(%d)]:\n",string,file,line);
   vfprintf(stderr,text,ap);
   fprintf(stderr,"\n");

   if(stat == GPM_STAT_OOPS) exit(1);  /* may a lib function call exit ???? */
}
