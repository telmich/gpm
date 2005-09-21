/*
 * optparser.h - GPM mouse options parser
 *
 * Copyright (C) 1993        Andrew Haylett <ajh@gec-mrc.co.uk>
 * Copyright (C) 1994-2000   Alessandro Rubini <rubini@linux.it>
 * Copyright (C) 1998,1999   Ian Zimmerman <itz@rahul.net>
 * Copyright (C) 2001,2002   Nico Schottelius <nico-gpm@schottelius.org>
 * Copyright (C) 2003        Dmitry Torokhov <dtor@mail.ru>
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
#ifndef __GPM_OPTPARSER_H_
#define __GPM_OPTPARSER_H_

enum option_type {
   OPT_BOOL = 1,
   OPT_INT, /* "%i" */
   OPT_DEC, /* "%d" */
   OPT_STRING,
   /* other types must be added */
   OPT_END = 0
};

struct option_helper {
   char *name;
   enum option_type type;
   union u {
      int *iptr;   /* used for int and bool arguments */
      char **sptr; /* used for string arguments, by strdup()ing the value */
   } u;
   int value; /* used for boolean arguments */
   int present;
};

int   parse_options(const char *who, const char *opt, char sep, struct option_helper *info);
int   check_no_options(const char *proto, const char *opts, char sep);
int   is_option_present(struct option_helper *info, const char *name);
#endif
