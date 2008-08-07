
/*
 * twiddler.c - support for the twiddler keyboard
 *
 * Copyright 1998          Alessandro Rubini (rubini at linux.it)
 * Copyright 2001 - 2008   Nico Schottelius (nico-gpm2008 at schottelius.org)
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


/* The functions and their name */
struct twiddler_fun_struct twiddler_functions[] = {
   {"Console", twiddler_console},
   {"Exec", twiddler_exec},
   {NULL, NULL}
};

/* The registered functions */
struct twiddler_active_fun {
   int (*fun) (char *s);
   char *arg;
} twiddler_active_funs[TWIDDLER_MAX_ACTIVE_FUNS];

static int active_fun_nr = 0;

#include "gpm.h"
#include "gpmInt.h"
#include "message.h"
#include "twiddler.h"

#include "daemon.h"

/*
 * Each table is made up of 256 entries, as these are the possible chords
 * Then, there is one table for each modifier (two of them are not allowed).
 * Each entry is a pointer: a "low" pointer represents a single byte, true
 * pointers represent strings. This is not very clean, but it works well.
 */

char *twiddler_table[7][256];

/* This maps modifiers to tables */
struct twiddler_map_struct {
   unsigned long modifiers;
   char *keyword;
   char **table;
} twiddler_map[] = {
   {
   TW_MOD_0, "", twiddler_table[0]}, {
   TW_MOD_S, "Shift", twiddler_table[1]}, {
   TW_MOD_N, "Numeric", twiddler_table[2]}, {
   TW_MOD_F, "Function", twiddler_table[3]}, {
   TW_MOD_C, "Control", twiddler_table[4]}, {
   TW_MOD_C, "Ctrl", twiddler_table[4]}, {
   TW_MOD_A, "Alt", twiddler_table[5]}, {
   TW_MOD_A, "Meta", twiddler_table[5]},

/* This is different!! */
   {
   TW_MOD_C | TW_MOD_S, "Ctrl+Shift", twiddler_table[6]}, {
   TW_MOD_C | TW_MOD_S, "Shift+Ctrl", twiddler_table[6]}, {
   0, NULL, NULL}
};

/* Convert special keynames to strings */
struct twiddler_f_struct {
   char *instring;
   char *outstring;
} twiddler_f[] = {
   {
   "F1", "\033[[A"}, {
   "F2", "\033[[B"}, {
   "F3", "\033[[C"}, {
   "F4", "\033[[D"}, {
   "F5", "\033[[E"}, {
   "F6", "\033[17~"}, {
   "F7", "\033[18~"}, {
   "F8", "\033[19~"}, {
   "F9", "\033[20~"}, {
   "F10", "\033[21~"}, {
   "F11", "\033[23~"}, {
   "F12", "\033[24~"}, {
   "F13", "\033[25~"}, {
   "F14", "\033[26~"}, {
   "F15", "\033[28~"}, {
   "F16", "\033[29~"}, {
   "F17", "\033[31~"}, {
   "F18", "\033[32~"}, {
   "F19", "\033[33~"}, {
   "F20", "\033[34~"}, {
   "Find", "\033[1~"}, {
   "Insert", "\033[2~"}, {
   "Remove", "\033[3~"}, {
   "Select", "\033[4~"}, {
   "Prior", "\033[5~"}, {
   "Next", "\033[6~"}, {
   "Macro", "\033[M"}, {
   "Pause", "\033[P"}, {
   "Up", "\033[A"}, {
   "Down", "\033[B"}, {
   "Left", "\033[D"}, {
   "Right", "\033[C"}, {
   NULL, NULL}
};

/* Convert special keynames to functions */
struct twiddler_fun_struct {
   char *name;
   int (*fun) (char *string);
};

#define TWIDDLER_MAX_ACTIVE_FUNS 128
