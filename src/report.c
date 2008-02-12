/*
 * selects where we should report to
 * TODO: - possibly include errno (%m) string in syslog
 *       - njak ... someone should optimize this code and remove double
 *         fragments.... (self)
 *
 * Copyright (c) 2001,2002   Nico Schottelius <nico@schottelius.org>
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
#include <stdlib.h>     /* alloc & co. */
#include <string.h>     /* strlen/strcpy */
#include <sys/types.h>  /* these three are */
#include <sys/stat.h>   /* needed for      */
#include <unistd.h>     /* stat() */

#include "headers/gpmInt.h"
#include "headers/message.h"

/*
 * gpm_report
 *
 * Destinations:
 * - syslog
 * - current console (/dev/vc/0 || /dev/tty0)
 * - system console (/dev/console)
 * - stdout
 * - stderr
 * 
 *
 * Items in [] are alternatives, if first destination is not present.
 *
 * Startup Mode:
 *    debug    :  - (ignore)
 *    info     :  syslog/stdout
 *    warn/err :  syslog/stderr
 *    oops     :  syslog/stderr  [exit]
 *
 * Running Mode: (daemon)
 *    debug    :  - (ignore)
 *    info     :  syslog
 *    warn     :  syslog+system console
 *    err      :  syslog+system console+current console
 *    oops     :  syslog/stderr  [_exit]
 *
 * Debug Mode  :
 *    debug/warn/err: stderr
 *    info          : stdout
 *    oops          : stderr     [exit]
 *
 * Client Mode: (to use with mouse-test etc. ; NON SERVER )
 *   debug/err/warn/info: like debugging mode!
 *
 */

void gpm_report(int line, char *file, int stat, char *text, ... )
{
   FILE *console = NULL;
   va_list ap;

   va_start(ap,text);

   switch(option.run_status) {
      /******************** STARTUP *****************/
      case GPM_RUN_STARTUP:
         switch(stat) {
            case GPM_STAT_INFO:
#ifdef HAVE_VSYSLOG
               syslog(LOG_INFO | LOG_USER, GPM_STRING_INFO);
               vsyslog(LOG_INFO | LOG_USER, text, ap);
#endif               
               fprintf(stderr,GPM_STRING_INFO);
               vfprintf(stderr,text,ap);
               fprintf(stderr,"\n");
               break;

            case GPM_STAT_WARN:
#ifdef HAVE_VSYSLOG
               syslog(LOG_DAEMON | LOG_WARNING, GPM_STRING_WARN);
               vsyslog(LOG_DAEMON | LOG_WARNING, text, ap);
#endif               
               fprintf(stderr,GPM_STRING_WARN);
               vfprintf(stderr,text,ap);
               fprintf(stderr,"\n");
               break;

            case GPM_STAT_ERR:
#ifdef HAVE_VSYSLOG
               syslog(LOG_DAEMON | LOG_ERR, GPM_STRING_ERR);
               vsyslog(LOG_DAEMON | LOG_ERR, text, ap);
#endif               
               fprintf(stderr,GPM_STRING_ERR);
               vfprintf(stderr,text,ap);
               fprintf(stderr,"\n");
               break;

            case GPM_STAT_OOPS:
#ifdef HAVE_VSYSLOG
               syslog(LOG_DAEMON | LOG_ERR, GPM_STRING_OOPS);
               vsyslog(LOG_DAEMON | LOG_ERR, text, ap);
#endif               
               fprintf(stderr,GPM_STRING_OOPS);
               vfprintf(stderr,text,ap);
               fprintf(stderr,"\n");

               exit(1); /* we should have a oops()-function,but this works,too*/
               break;
         }
         break; /* startup sequence */   

      /******************** RUNNING *****************/
      case GPM_RUN_DAEMON:
         switch(stat) {
            case GPM_STAT_INFO:
#ifdef HAVE_VSYSLOG
               syslog(LOG_INFO | LOG_USER, GPM_STRING_INFO);
               vsyslog(LOG_INFO | LOG_USER, text, ap);
#endif
               break;

            case GPM_STAT_WARN:
#ifdef HAVE_VSYSLOG
               syslog(LOG_DAEMON | LOG_WARNING, GPM_STRING_WARN);
               vsyslog(LOG_DAEMON | LOG_WARNING, text, ap);
#endif               
               if((console = fopen(GPM_SYS_CONSOLE,"a")) != NULL) {
                  fprintf(console,GPM_STRING_WARN);
                  vfprintf(console,text,ap);
                  fprintf(console,"\n");
                  fclose(console);
               }   
               break;
 
            case GPM_STAT_ERR:
#ifdef HAVE_VSYSLOG
               syslog(LOG_DAEMON | LOG_ERR, GPM_STRING_ERR);
               vsyslog(LOG_DAEMON | LOG_ERR, text, ap);
#endif               
               if((console = fopen(GPM_SYS_CONSOLE,"a")) != NULL) {
                  fprintf(console,GPM_STRING_ERR);
                  vfprintf(console,text,ap);
                  fprintf(console,"\n");
                  fclose(console);
               }

               if((console = fopen(option.consolename,"a")) != NULL) {
                  fprintf(console,GPM_STRING_ERR);
                  vfprintf(console,text,ap);
                  fprintf(console,"\n");
                  fclose(console);
               }
               break;

            case GPM_STAT_OOPS:
#ifdef HAVE_VSYSLOG
               syslog(LOG_DAEMON | LOG_ERR, GPM_STRING_OOPS);
               vsyslog(LOG_DAEMON | LOG_ERR, text, ap);
#endif               
               fprintf(stderr,GPM_STRING_OOPS);
               vfprintf(stderr,text,ap);
               fprintf(stderr,"\n");

               _exit(1); /* we are the fork()-child */
               break;
         }
         break; /* running gpm */

      /******************** DEBUGGING and CLIENT *****************/
      case GPM_RUN_DEBUG:
         switch(stat) {
            case GPM_STAT_INFO:
               console = stdout;
               fprintf(console,GPM_STRING_INFO); break;
            case GPM_STAT_WARN:
               console = stderr;
               fprintf(console,GPM_STRING_WARN); break;
            case GPM_STAT_ERR:
               console = stderr;
               fprintf(console,GPM_STRING_ERR); break;
            case GPM_STAT_DEBUG:
               console = stderr;
               fprintf(console,GPM_STRING_DEBUG); break;
            case GPM_STAT_OOPS:
               console = stderr;
               fprintf(console,GPM_STRING_OOPS); break;
         }

         vfprintf(console,text,ap);
         fprintf(console,"\n");
         
         if(stat == GPM_STAT_OOPS) exit(1);

         break;
   } /* switch for current modus */
} /* gpm_report */   


/* old interesting part from debuglog.c.
 * interesting, if you want to include ERRNO into syslog message
 * should possibly included again later...when sources are clean and
 * there is no doubled strrer(errno) 

#if(defined(HAVE_VSYSLOG) && defined(HAVE_SYSLOG))
    static char fmt[] = ": %m";
    char* buf = alloca(strlen(s)+sizeof(fmt));
    strcpy(buf, s); strcat(buf, fmt);
#endif

*/
