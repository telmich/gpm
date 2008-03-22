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

#include <stdlib.h>                 /* atoi              */
#include <unistd.h>                 /* getopt            */

#include "headers/message.h"        /* messaging in gpm */
#include "headers/daemon.h"         /* daemon internals */

/*****************************************************************************
 * Check the command line and / or set the appropriate variables
 * Can't believe it, today cmdline() really does what the name tries to say
 *****************************************************************************/
void cmdline(int argc, char **argv)
{
   extern struct options option;
   char options[]="a:A::b:B:d:Dg:hi:kl:m:Mo:pr:R::s:S:t:Tuv23";
   int  opt;

   /* initialize for the dual mouse */ 
   mouse_table[2]=mouse_table[1]=mouse_table[0]; /* copy defaults */
   which_mouse = mouse_table+1; /* use the first */

   while ((opt = getopt(argc, argv, options)) != -1) {
      switch (opt) {
         case 'a': (which_mouse->opt_accel) = atoi(optarg);             break;
         case 'A': opt_aged++;
                   if (optarg)
                     opt_age_limit = atoi(optarg);       break;
         case 'b': (which_mouse->opt_baud) = atoi(optarg);              break;
         case 'B': (which_mouse->opt_sequence) = optarg;                break;
         case 'd': (which_mouse->opt_delta) = atoi(optarg);             break;
         case 'D': option.run_status = GPM_RUN_DEBUG;    break;
         case 'g': (which_mouse->opt_glidepoint_tap)=atoi(optarg);      break;
         case 'h': exit(usage(NULL));
         case 'i': (which_mouse->opt_time)=atoi(optarg);                break;
         case 'k': check_kill();                         break;
         case 'l': opt_lut = optarg;                     break;
         case 'm': add_mouse(GPM_ADD_DEVICE,optarg);     
                   (which_mouse->opt_dev) = optarg;                     break; /* GO AWAY!*/
         case 'M': opt_double++; option.repeater++;
            if (option.repeater_type == 0)
               option.repeater_type = "msc";
            which_mouse=mouse_table+2;                   break;
         case 'o': add_mouse(GPM_ADD_OPTIONS,optarg);
                   gpm_report(GPM_PR_DEBUG,"options: %s",optarg);
                   (which_mouse->opt_options) = optarg;                 break; /* GO AWAY */
         case 'p': opt_ptrdrag = 0;                      break;
         case 'r':
            /* being called responsiveness, I must take the inverse */
            (which_mouse->opt_scale)=atoi(optarg);
            if(!(which_mouse->opt_scale) || (which_mouse->opt_scale) > 100) (which_mouse->opt_scale)=100; /* the maximum */
            else (which_mouse->opt_scale)=100/(which_mouse->opt_scale);                break;
         case 'R':
            option.repeater++;
            if (optarg) option.repeater_type = optarg;
            else        option.repeater_type = "msc";    break;
         case 's': (which_mouse->opt_sample) = atoi(optarg);            break;
         case 'S': if (optarg) opt_special = optarg;
                   else opt_special="";                  break;
         case 't': add_mouse(GPM_ADD_TYPE,optarg);
                   (which_mouse->opt_type) = optarg;                    break; /* GO AWAY */
         case 'u': option.autodetect = 1;                break;
         case 'T': opt_test++;                           break;
         case 'v': printf(GPM_MESS_VERSION "\n");        exit(0);
         case '2': (which_mouse->opt_three) = -1;                       break;
         case '3': (which_mouse->opt_three) =  1;                       break;
         default: exit(usage("commandline"));
      }
   }
}
