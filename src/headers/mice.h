/*
 * Mice related
 *
 * Copyright (c) 2008    Nico Schottelius <nico-gpm2008 at schottelius.org>
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

#ifndef _GPM_MICE_H
#define _GPM_MICE_H

/*************************************************************************
 * Includes
 */
#include "gpm.h"           /* Gpm_Event         */
#include "types.h"         /* Gpm_Type          */


/*************************************************************************
 * Macros
 */

#define STD_FLG (CREAD|CLOCAL|HUPCL)
#define GPM_B_BOTH (GPM_B_LEFT|GPM_B_RIGHT)
#define REALPOS_MAX 16383 /*  min 0 max=16383, but due to change.  */

/*** mouse commands ***/
#define GPM_AUX_SEND_ID    0xF2
#define GPM_AUX_ID_ERROR   -1
#define GPM_AUX_ID_PS2     0
#define GPM_AUX_ID_IMPS2   3

/* these are shameless stolen from /usr/src/linux/include/linux/pc_keyb.h    */
#define GPM_AUX_SET_RES        0xE8  /* Set resolution */
#define GPM_AUX_SET_SCALE11    0xE6  /* Set 1:1 scaling */
#define GPM_AUX_SET_SCALE21    0xE7  /* Set 2:1 scaling */
#define GPM_AUX_GET_SCALE      0xE9  /* Get scaling factor */
#define GPM_AUX_SET_STREAM     0xEA  /* Set stream mode */
#define GPM_AUX_SET_SAMPLE     0xF3  /* Set sample rate */
#define GPM_AUX_ENABLE_DEV     0xF4  /* Enable aux device */
#define GPM_AUX_DISABLE_DEV    0xF5  /* Disable aux device */
#define GPM_AUX_RESET          0xFF  /* Reset aux device */
#define GPM_AUX_ACK            0xFA  /* Command byte ACK. */

#define GPM_AUX_BUF_SIZE    2048  /* This might be better divisible by
                                 three to make overruns stay in sync
                                 but then the read function would need
                                 a lock etc - ick */

#define GPM_EXTRA_MAGIC_1 0xAA
#define GPM_EXTRA_MAGIC_2 0x55

#define  abs(value)              ((value) < 0 ? -(value) : (value))

/*************************************************************************
 * Global variables
 */
extern int realposx;
extern int realposy;


/*************************************************************************
 * Mice functions
 */

int check_no_argv(int argc, const char **argv);
int option_modem_lines(int fd, int argc, const char **argv);
int parse_argv(argv_helper *info, int argc, const char **argv);
int read_mouse_id(int fd);
int setspeed(int fd,int old,int new,int needtowrite,unsigned short flags);
int write_to_mouse(int fd, unsigned char *data, size_t len);

#endif
