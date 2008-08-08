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

#ifndef _GPM_TWIDDLER_H
#define _GPM_TWIDDLER_H

/* structs */

/* Convert special keynames to functions */
struct twiddler_fun_struct {
   char *name;
   int (*fun) (char *string);
};


/* definition of the twiddler protocol */

/*
 * 2400,8,n,1  5-bytes
 *
 *  bit    7   6   5   4   3   2   1   0
 *         0  4L  3M  3L  2M  2L  1M  1L
 *         1  Mo  Al  Co  Fn  Nm  Sh  4M
 *         1  V6  V5  V4  V3  V2  V1  V0
 *         1  H4  H3  H2  H1  H0  V8  V7
 *         1   0   0   0  H8  H7  H6  H5
 * H and V are two complement: up and left is positive
 */

#define TW_L1 0x0001
#define TW_M1 0x0002
#define TW_R1 0x0003
#define TW_ANY1 0x0003
#define TW_L2 0x0004
#define TW_M2 0x0008
#define TW_R2 0x000C
#define TW_ANY2 0x000C
#define TW_L3 0x0010
#define TW_M3 0x0020
#define TW_R3 0x0030
#define TW_ANY3 0x0030
#define TW_L4 0x0040
#define TW_M4 0x0080
#define TW_R4 0x00C0
#define TW_ANY4 0x00C0

#define TW_MOD_0   0x0000
#define TW_MOD_S   0x0100
#define TW_MOD_N   0x0200
#define TW_MOD_F   0x0400
#define TW_MOD_C   0x0800
#define TW_MOD_A   0x1000
#define TW_MOD_M   0x2000

#define TW_ANY_KEY 0x3fff /* any button or modifier */
#define TW_ANY_MOD 0x3f00 /* any modifier */


#define TW_V_SHIFT 14
#define TW_H_SHIFT 23
#define TW_M_MASK 0x1FF /* mask of movement bits, after shifting */
#define TW_M_BIT  0x100

#define TW_SYSTEM_FILE SYSCONFDIR "/gpm-twiddler.conf"
#define TW_CUSTOM_FILE SYSCONFDIR "/gpm-twiddler.user"

int twiddler_chord_to_int(char *chord);
int twiddler_console(char *s);
int twiddler_do_fun(int i);
int twiddler_escape_sequence(char *s, int *len);
int twiddler_exec(char *s);
char **twiddler_get_table(unsigned long message);
char **twiddler_mod_to_table(char *mod);
char *twiddler_rest_to_value(char *s);
int twiddler_use_item(char *item);

int twiddler_key(unsigned long message);
int twiddler_key_init(void);

#define TWIDDLER_MAX_ACTIVE_FUNS 128

#endif
