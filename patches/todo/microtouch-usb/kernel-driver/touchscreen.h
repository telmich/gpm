/* -*- linux-c -*- */

/* 
 * Driver for USB Touchscreen (Microtech - IBM SurePos 4820)
 *
 * Copyright (C) 2000 Wojciech Woziwodzki
 * Written by Radoslaw Garbacz
 *
 * The header file
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Originally based upon scanner.c (David E. Nelson).
 *
 */


#include <linux/types.h>
#include <linux/wait.h>


// CTRL numbers of commands define
#define TSCRN_SOFT_RESET        73
#define TSCRN_HARD_RESET        74
#define TSCRN_CALIBRATE         75
#define TSCRN_CONTROLLER_STATUS 76
#define TSCRN_RESTORE_DEFAULTS  77
#define TSCRN_CONTROLLER_ID     78
#define TSCRN_REPORT_ENABLE     79
#define TSCRN_CLEAR_HALT        80
#define TSCRN_USER_REQUEST      81
#define TSCRN_CALIBRATION       82
// device requests define
#define TSCRN_USB_REQUEST_RESET         7
#define TSCRN_USB_REQUEST_CALIBRATION   4
#define TSCRN_USB_REQUEST_STATUS        6
#define TSCRN_USB_REQUEST_RESTORE_DEFAULTS 8
#define TSCRN_USB_REQUEST_CONTROLLER_ID 10
// define requests parameters
//        for reset request
#define TSCRN_USB_PARAM_SOFT_RESET      1
#define TSCRN_USB_PARAM_HARD_RESET      2
//        for calibrate request
#define TSCRN_EXTENDED_CALIBRATION_TYPE        1
#define TSCRN_CORNER_CALIBRATION_TYPE           2

// define raport sizes
#define TSCRN_USB_RAPORT_SIZE_DATA      11
#define TSCRN_USB_RAPORT_SIZE_STATUS    8
#define TSCRN_USB_RAPORT_SIZE_ID        16

#define IBUF_SIZE TSCRN_USB_RAPORT_SIZE_ID
//input data buffer 12 * data raports
#define OBUF_SIZE TSCRN_USB_RAPORT_SIZE_DATA*12


#define MICROTOUCH_VENDOR_ID	0x0596
#define MICROTOUCH_PRODUCT_ID	0x0001

//the report no 1 field definitions
#define IS_TOUCHED_BYTE(data) ((data & 0x40) ? 1:0)
#define IS_TOUCHED(str) ((str->PenStatus & 0x40) ? 1:0)
#define GET_XC(str)     (str->XCompensHi<<8 | str->XCompensLo)
#define GET_YC(str)     (str->YCompensHi<<8 | str->YCompensLo)
#define GET_XR(str)     (str->XRawHi<<8 | str->XRawLo)
#define GET_YR(str)     (str->YRawHi<<8 | str->YRawLo)
//#define GET_XC(str)     (str->XCompensHi*256+str->XCompensLo)
//#define GET_YC(str)     (str->YCompensHi*256+str->YCompensLo)
//#define GET_XR(str)     (str->XRawHi*256+str->XRawLo)
//#define GET_YR(str)     (str->YRawHi*256+str->YRawLo)
#define GET_LOOP(str)   (str->LoopCounter)

struct tscrn_usb_data_report
{
  __u8 Id;             // 0x01 for this report
  __u8 LoopCounter;
  __u8 PenStatus;      // 7b = 1; 6b is 1 for touching, 0 when not touching
  __u8 XCompensLo;     //compensed low 8 bits of the X coordinate
  __u8 XCompensHi;     //          high              X          
  __u8 YCompensLo;     //          low               Y
  __u8 YCompensHi;     //          high              Y
  __u8 XRawLo;         //raw low 8 bits of the X coordinate
  __u8 XRawHi;         //    high              X
  __u8 YRawLo;         //    low               Y
  __u8 YRawHi;         //    high              Y
};

struct tscrn_usb_status_report
{
  __u8  Id;            // 0x06
  __u16 POCStatus;     // power on check status
  __u8  CMDStatus;     // last commad status
  __u8  TouchStatus;   // finger up/down
  __u8  Filter[3];     // filter, for future expansion
};

struct tscrn_usb_controller_id_raport
{
  __u8  Id;              // 0x0C
  __u16 ControllerType;  // Controller type 
  __u8  FWMajorRevision; // firmware major revision level
  __u8  FMMinorRevision; // firmware minor revision level
  __u8  Features;        // special features
  __u8  ROMChecksum;     // ROM Checksum
  __u16 Reserved01;
  __u8  Reserved02;
  __u8  Reserved03;
  __u8  Reserved04;
  __u8  Reserved05;
  __u8  Reserved06;
  __u8  Reserved07;
};


