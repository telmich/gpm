/*
 * gpn.c - support functions for gpm-Linux
 *
 * Copyright (C) 1993        Andreq Haylett <ajh@gec-mrc.co.uk>
 * Copyright (C) 1994-1999   Alessandro Rubini <rubini@linux.it>
 * Copyright (C) 1998 	     Ian Zimmerman <itz@rahul.net>
 * Copyright (c) 2001-2005   Nico Schottelius <nico-gpm@schottelius.org>
 *
 * Tue,  5 Jan 1999 23:26:10 +0000, modified by James Troup <james@nocrew.org>
 * (usage): typo (s/an unexistent/a non-existent/)
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
#include <string.h>        /* strerror(); ?!? memcpy() */
#include <ctype.h>         /* isdigit */
#include <unistd.h>        /* getopt(),symlink() */

#include "headers/message.h"
#include "headers/gpmInt.h"
#include "headers/gpm.h"
#include "headers/console.h"
#include "headers/selection.h"

/* usage: display for usage informations */
int usage(char *whofailed)
{
   if (whofailed) {
      gpm_report(GPM_PR_ERR, GPM_MESS_SPEC_ERR, whofailed, option.progname);
      return 1;
   }
   printf(GPM_MESS_USAGE, option.progname, DEF_ACCEL, DEF_BAUD, DEF_SEQUENCE,
          DEF_DELTA, DEF_TIME, DEF_LUT, DEF_SCALE, DEF_SAMPLE, DEF_TYPE);
   return 1;
}

/*****************************************************************************
 * the function returns a valid type pointer or NULL if not found
 *****************************************************************************/
static struct Gpm_Type *find_mouse_by_name(char *name)
{
   Gpm_Type *type;
   char *s;
   int len = strlen(name);

   for (type = mice; type->fun; type++) {
      if (!strcasecmp(name, type->name)) break;
      /* otherwise, look in the synonym list */
      for (s = type->synonyms; s; s = strchr(s, ' ')) {
         while (*s && isspace(*s)) s++; /* skip spaces */
         if (!strncasecmp(name, s, len) && !isprint(*(s + len))) break;/*found*/
      }
   	if (s) break; /* found a synonym */
   }
   return type->fun ? type : NULL;
}

static void init_button_sequence(struct miceopt *opt, char *arg)
{
   int i;
   static struct {
      char *in;
      char *out;
   } seq[] = {
      {"123", "01234567"},
      {"132", "02134657"},
      {"213", "01452367"}, /* warning: these must be readable as integers... */
      {"231", "02461357"},
      {"312", "04152637"},
      {"321", "04261537"},
      {"111","04444444"}, /* jwz: this allows one to map all three buttons */
      {"222","02222222"}, /* to button1. */
      {"333","01111111"},
      {NULL, NULL}
   };

   if (strlen(arg) != 3 || atoi(arg) < 100)
      exit(usage("sequence"));

   for (i = 0; seq[i].in && strcmp(seq[i].in, arg); i++) ;

   if (!seq[i].in)
      exit(usage("button sequence"));

   opt->sequence = strdup(seq[i].out); /* I can rewrite on it */
}

static void validate_mouse(struct micetab *mouse, int mouse_no)
{
   if (!mouse->device) {
      if (!mouse->type && mouse_no > 1)
         gpm_report(GPM_PR_OOPS,
            "No device/protocol specified for mouse #%d, probably extra -M option?", mouse_no);
      else
         gpm_report(GPM_PR_OOPS, "No device specified for mouse #%d", mouse_no);
   }

   if (!mouse->type)
      mouse->type = find_mouse_by_name(DEF_TYPE);

   mouse->options.absolute = mouse->type->absolute;

   if (!mouse->options.sequence)
      init_button_sequence(&mouse->options, DEF_SEQUENCE);
}

static void validate_repeater(char *type)
{
   if (strcmp(type, "raw") == 0)
      repeater.raw = 1;
   else {
      repeater.raw = 0;

      if (!(repeater.type = find_mouse_by_name(type)))
         exit(M_listTypes()); /* not found */

      if (!repeater.type->repeat_fun) /* unsupported translation */
         gpm_report(GPM_PR_OOPS, GPM_MESS_NO_REPEAT, type);
   }
}

/*****************************************************************************
 * Check the command line and / or set the appropriate variables
 * Can't believe it, today cmdline() really does what the name tries to say
 *****************************************************************************/
void cmdline(int argc, char **argv)
{
   struct micetab *mouse;
   struct miceopt *opt;
   char options[]="a:A::b:B:d:Dfg:hi:kl:m:Mo:pr:R::s:S:t:TuvV::23";
   int  opt_char, tmp;
   int  mouse_no = 1;

   mouse = add_mouse();
   opt = &mouse->options;

   while ((opt_char = getopt(argc, argv, options)) != -1) {
      switch (opt_char) {
         case 'a':   if ((opt->accel = atoi(optarg)) < 1)
                        exit(usage("acceleration"));
                     break;
         case 'A':   sel_opts.aged = 1;
                     if (optarg)
                        sel_opts.age_limit = atoi(optarg);
                     break;
         case 'b':   opt->baud = atoi(optarg);
                     break;
         case 'B':   init_button_sequence(opt, optarg);
                     break;
         case 'd':   if ((opt->delta = atoi(optarg)) < 2)
                        exit(usage("delta"));
                     break;
         case 'D':   option.run_status = GPM_RUN_DEBUG;
                     break;
         case 'f':   option.run_status = GPM_RUN_FORK;
                     break;
         case 'g':   if (atoi(optarg) > 3)
                        exit(usage("glidepoint tap button"));
                     opt->glidepoint_tap = GPM_B_LEFT >> (atoi(optarg) - 1);
                     break;
         case 'h':   exit(usage(NULL));
         case 'i':   opt->time = atoi(optarg);
                     break;
         case 'k':   kill_gpm();
                     break;
         case 'l':   console.charset = optarg;
                     break;
         case 'm':   mouse->device = optarg;
                     break;
         case 'M':   validate_mouse(mouse, mouse_no);
                     mouse = add_mouse();
                     opt = &mouse->options;
                     mouse_no++;
                     if (!repeater.type && !repeater.raw)
                        repeater.type = find_mouse_by_name(DEF_REP_TYPE);
                     break;
         case 'o':   gpm_report(GPM_PR_DEBUG,"options: %s", optarg);
                     opt->text = optarg;
                     break;
         case 'p':   sel_opts.ptrdrag = 0;
                     break;
         case 'r':   /* being called responsiveness, I must take the inverse */
                     tmp = atoi(optarg);
                     if (!tmp || tmp > 100) tmp = 1;
                     opt->scalex = 100 / tmp;
                     break;
         case 'R':   validate_repeater((optarg) ? optarg : DEF_REP_TYPE);
                     break;
         case 's':   opt->sample = atoi(optarg);
                     break;
         case 'S':   if (optarg) opt_special = optarg;
                     else opt_special="";
                     break;
         case 't':   mouse->type = find_mouse_by_name(optarg);
                     if (!mouse->type)
                        exit(M_listTypes());
                     break;
         case 'u':   option.autodetect = 1;
                     break;
         case 'v':   printf(GPM_MESS_VERSION "\n");
                     exit(0);
         case '2':   opt->three_button = -1;
                     break;
         case '3':   opt->three_button = 1;
                     break;
         default:    exit(usage("commandline"));
      }
   }

   validate_mouse(micelist, mouse_no);
}
