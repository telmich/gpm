/*
 * synaptics.c - support for the synaptics serial and ps2 touchpads
 *
 * Copyright 1999   hdavies@ameritech.net (Henry Davies)
 *                  Geert Van der Plas provided the code to support
 *                        older Synaptics PS/2 touchpads.
 *
 *   Synpatics Passthrough Support Copyright (C) 2002 Linuxcare Inc.
 *   dkennedy@linuxcare.com (David Kennedy)
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

/*
** Design/Goals
**     I want to use the Synaptics Serial touchpad as a cursor device under
**     Linux (gpm).  With this device I want to support operations similar
**     to those supported by the Synaptics WinDOS driver, and some others 
**     of my own devising.
**
** Features:
**     Corner Clicks
**         This recognizes taps on the touchpad in the corner(s) and
**         translates them into specific actions.  Initially I am looking
**         at actions on the order of alternate button clicks.  Other 
**         alternatives include drags and whatnot.
**     Edge Extensions
**         This recognizes that the finger has moved from the center region
**         of the touchpad and dragged to the edge area.  At which point
**         I want to be able to extend the motion by automatically moving 
**         in the direction of the edge.
**     Toss n Catch
**         This recognizes a quick motion of the finger on the touchpad and
**         uses that to define a velocity vector for the cursor.  A tap
**         on the touchpad at a later time catches (stops) the cursor.
**     Tap n Drag
**         A quick tap of the touchpad followed by finger motion on the 
**         touchpad initiates what would be a drag with a normal mouse
**         type device.
**     Pressure Sensitive Velocity
**         The Synaptics touchpad indicates the touch pressure of the finger
**         (really an interface area) this is used to accelerate the cursor
**         motion.  This can be used in the normal motion, Tap n Drag, or
**         Edge Extension modes.  In normal motion and Tap n Drag this may 
**         be awkward due to increased friction caused by the pressure.
**
** Parameters:
**     search below for configuration constants
*/


/* Pebl - pebl@diku.dk 20/12/2001 08:06.
**
** I want to have more features :). Every touch pad (ps2 and serial) is now be
** supported according to STIG and every feature/capability is read.  However
** there seems to be some very old serial touchpads that have an advanced
** mode not mentioned in STIG.
**
**
** Further I have added the following
**
**  Emulation of scrolling
**        The window drivers allow using the edge to emulate a wheel
**        mouse. When putting the finger at the right edge, a movement up or
**        down translates to wheel movement. Taking the finger to the
**        top/button edge keeps the wheel turning.  Lift the finger again to
**        operate normally again. Another option is to do a toss scrolling.
**
**  Stick attached to a synaptic touchpad (aka styk)
**        Some touchpads have a stick "attached", so they share the same port.
**        In absolute mode the stick protocol is a simple ps2 protocol, except
**        it is sent in an absolute packet. (In relative mode both uses ps2)
**        Some sticks send packets when pressed (different from moved), which
**        will be reported as left button.
**
**  4 Way button attached to a touchpad
**        Likewise some touchpads have a round button giving the choices of
**        four way to press the button. The packet is nearly identical to the
**        stick, turn off the stick if you have this button. This is no longer
**        needed.  The button can act as a mouse or as buttons which can be
**        changed on the fly. The buttons can be configured just as the
**        touchpads corners.
**
**  Four buttons to work
**        Some touchpads have 4 buttons. Only 3 was read and the last was set to 
**        up or down "button". They can be configured just as the
**        touchpads corners.
**
**  Multiple fingers
**        I have added an option to detect such by looking at pressure levels,
**        which is not that great. If the pad have the capability to detect it,
**        this is used instead, but this far from optimal. It does detect two
**        horizontal parted finger if in same vertical position, otherwise the
**        detection is bad. This is/was a problem as my wrist lays on the
**        laptop which gives a 45 deg to horizontal for my fingers. Now I try
**        to add further detections, which works for me in 95% of the
**        time. 
**
**  Multi finger tap
**        Using 1,2 or 3 fingers to make a tap translates into left, right or
**        middle button pressed. I originally thought about doing what is
**        called a "HOP" where after a quick finger shift the distance of the
**        hop decides which button is pressed. I dropped it partly because
**        synaptics did in version 3.2 and because it was harder than I
**        thought. Multifinger taps can be configured just as the touchpads corners.
**
**  Multi finger less sensitive
**        Adding or removing a finger or just accidently touch the pad with
**        the palm while using the pad causes the pad to report the average
**        position between the touched places.  This of cause gives annoying
**        erratic movements. When adding or removing fingers are detected
**        the mouse is stop for some small time to avoid this. This is not
**        perfect as the finger detection is not perfect, but it helps a lot!
**
**  Palm detection
**        Some touchpads tries to detect whether the reported data is from a
**        palm or similar.  When reading this signal the movement is
**        stopped. Except the detection does not work very well :(
**
**
**  Enabling/disabling touchpad
**        A new corner action is to disable the touchpad. It is enabled again
**        by tapping the same corner. This is useful if I know I am going
**        to write a lot. The stick still works.
**
**  Debugging corner action  
**        A corner action now allows toggling the debug information. 
**
**
** The structure has changed to handle both serial and ps2 touchpads. Many
** variable names have changed.                  
**
** The documentation referred to is from
**  "Synaptics TouchPad Interfacing Guide" revision 2.5 
** or just STIG
**                                                                                 
**/


/*
** TODO
**   - handle other versions of synaptics touchpads (mine is 3.4 firmware)
**     (this should be close)
**   - test this with more Synaptics touch pads (laptops and such)
**   - provide a configuration interface to adjust parameters and
**     enabled features
**   - determine appropriate ranges for adjusting parameters
**
** TODO (pebl)
**   - Two pads can not be used at the same time (internal/external)
**   - Move a lot of the comments to a readme file.
**   - Allow normal taps in unused corners. 
**   - Better detection of spontaneous reseting touchpad.
**   - Implement resend command for serial connection.
**   - Only start scrolling if mostly vertical movement.
**   - Disable/enable command for serial touchpad.
**   - Clean up the mess about extended packets with pressure 0.
**   - source splitting of normal touchpad functions and synaptic specific and
**     add other touchpads.
*/



/**
** Notation:
** A gesture means a motion or action that is not a regular mouse movement.
** 
** Wmode is an absolute mode where no gesture is signaled by the touchpad, only
** supported on newer versions.
**
** variables *_enabled are use for enable behaviors that may not supported in
** the touchpad.
**
** Many variables have two counterparts (and some that dont should have): a
** *_time and *_packet, where the first is in msec and the latter is the same
** time converted to the number of packets, when using 80 packets per second.
**
** Variables:
** last_* is for last reviewed packet, not last reported/calculated movement/event.
** was_*  is for last event, not necessary last packet.
**
**
** Data process line:
**
** syn_process_ps2_data   ---- syn_translate_serial_data    --                         -----------------------------------------------
**                                (handles also wmode)       |                         |                            |
**                                                           |                         |                            V
**                                                           |- syn_preprocess_report --- syn_process_wmode_report --- syn_process_report
**                                                           |                          
** syn_process_serial_data --- syn_translate_ps2_wmode_data --                                                       
**                          |                                |                          
**                          |                                |                          
**                          -- syn_translate_ps2_data     ----                          
**
**
*/


#include <math.h>                /* ceil */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include "headers/gpm.h"
#include "headers/gpmInt.h"       /* which_mouse */
#include "headers/synaptics.h"
#include "headers/message.h"

#define DEBUG                     1
#undef DEBUG

#ifdef DEBUG
#  define DEBUG_SYNAPTIC          
#  define DEBUG_PARAMS            1
#  define DEBUG_FLUSH             1
#  define DEBUG_RESET             1
#  define DEBUG_SENT_DATA         1
#  define DEBUG_RECEIVED_DATA     1
#  define DEBUG_TOSS              1
#  define DEBUG_PALM              1
#  define DEBUG_STICK             1
#  define DEBUG_TAPS              1
#  define DEBUG_MULTI_FINGER      1
#  define DEBUG_CMD               1
#  define DEBUG_GETBYTE           1    /* this is VERY verbose */
#  undef  DEBUG_PUTBYTE           1    /* this is VERY verbose */
#  define DEBUG_PUTBYTE_ACK       1    /* this is VERY verbose */
#  define DEBUG_REPORTS           1    /* this is VERY verbose */
#endif


/* The next is UGLY, but I only want debug info from this file */
/* it's really ugly and you shouldn't use this. Instead use
 * gpm -D | grep filename.c.
 * ifdef DEBUG should not be used anymore! */
/* pebl: But this require that I boot my machine with gpm -D which I dont. The
   intention is that when I am doing my normal doing, and the touchpad suddenly
   behaves strange, I can start debuging without restarting gpm, using a
   corneraction.
*/

/* BAD CODE BEGIN 
#ifdef DEBUG_SYNAPTIC
#  include <stdarg.h>
   static int debug_syn_to_stderr = 1;

   static void gpm_report_static(int line, char *file, int stat, char* fmt, ... ) {

     va_list ap;
     va_start(ap, fmt);
     if (debug_syn_to_stderr){
       vfprintf(stderr, fmt, ap);
       fputs("\n", stderr);
     }else{
       gpm_report(line,file,stat,fmt,&ap);
     }
     va_end(ap);

   }
#  define gpm_report gpm_report_static
#endif

* END BADE CODE */

/* Prototype */
typedef unsigned char byte;

static void syn_ps2_absolute_mode(int fd);
static char *syn_model_name (int sensor);
static void syn_ps2_send_cmd(int fd, int stick, byte cmd); 


/* Defines */

#define abs(value)              ((value) < 0 ? -(value) : (value))
#define check_bits(value,mask)  (((value) & (mask)) == (mask))
#define sqr(value)              ((value) * (value))
#define distance(dx,dy)         (sqr(dx) + sqr(dy))
#define mod4(value)             ((value) % 4)
#define max(x,y)                ((x)>(y)?(x):(y))

/*
** Define the edge bit values.
*/
#define LEFT_EDGE           0x01
#define RIGHT_EDGE          0x02
#define TOP_EDGE            0x04
#define BOTTOM_EDGE         0x08
#define UPPER_LEFT_CORNER   (LEFT_EDGE  | TOP_EDGE)
#define LOWER_LEFT_CORNER   (LEFT_EDGE  | BOTTOM_EDGE)
#define UPPER_RIGHT_CORNER  (RIGHT_EDGE | TOP_EDGE)
#define LOWER_RIGHT_CORNER  (RIGHT_EDGE | BOTTOM_EDGE)
#define is_corner(edges)    (((edges) & (LEFT_EDGE | RIGHT_EDGE)) && \
			     ((edges) & (TOP_EDGE  | BOTTOM_EDGE)))

/*
** Define the action button bit values.
*/
#define RIGHT_BUTTON        0x01
#define MIDDLE_BUTTON       0x02
#define LEFT_BUTTON         0x04
#define FOURTH_BUTTON       0x08
#define UP_BUTTON           0x10
#define DOWN_BUTTON         0x20
#define ONE_FINGER          0x01
#define TWO_FINGERS         0x02
#define THREE_FINGERS       0x04
#define FOUR_UP_BUTTON      0x01
#define FOUR_DOWN_BUTTON    0x02
#define FOUR_LEFT_BUTTON    0x04
#define FOUR_RIGHT_BUTTON   0x08
#define STICK_RIGHT_BUTTON  0x01
#define STICK_MIDDLE_BUTTON 0x02
#define STICK_LEFT_BUTTON   0x04
/*
** Define additional gpm button bit values.
*/
#define GPM_B_NOT_SET       (-1)

/*
** Define the guest and touchpad devices.
*/
#define DEVICE_TOUCHPAD     0
#define DEVICE_STICK        1


/****************************************************************************
**
** Configuration constants.
**
** Those with C at the end are suitable for adjustment by a configuration
** program.
****************************************************************************/

static int corner_taps_enabled         = 1;  /* are corner taps enabled C*/
static int tap_gesture_enabled         = 1;  /* are gestures treaded as taps enabled  C*/
static int tossing_enabled             = 1;  /* is toss/catch enabled   C*/
static int does_toss_use_static_speed  = 1;  /* is toss/catch speed     C*/
                                                /* based on toss dist       */
static int edge_motion_enabled         = 1;  /* is edge motion enabled  C*/
static int edge_motion_speed_enabled   = 1;  /* does pressure control   C*/
                                               /* speed of edge motion     */
static int pressure_speed_enabled      = 1;  /* does pressure control   C*/
                                                 /* speed in non edges       */
static int tap_hold_edge_motion_enabled = 1; /* Enable edge motion while holding a tap  C*/

/* pressure induced speed related configuration constants */
static int   low_speed_pressure      = 60;                                /*C*/
static int   speed_up_pressure       = 60;                                /*C*/
static float speed_pressure_factor   = 0.05;                              /*C*/
static float standard_speed_factor   = 0.08;                              /*C*/

/* toss/catch related constants */
static int   min_toss_time           = 100;   /* ms: 0.10 sec */
static int   max_toss_time           = 300;   /* ms: 0.30 sec */          /*C*/
static int   prevent_toss_time       = 300;   /* ms: 0.25 sec */          /*C*/
static int   min_toss_dist           = 2;     /* mm */                    /*C*/
static int   static_toss_speed       = 70;                                /*C*/
static float toss_speed_factor       = 0.5;                               /*C*/

/* edge motion related configuration constants */
static int   x_min_center            = 1632;  /* define left edge           C*/
static int   x_max_center            = 5312;  /* define right edge          C*/
static int   y_min_center            = 1408;  /* define bottom edge         C*/
static int   y_max_center            = 4108;  /* define top edge            C*/
static int   edge_speed              = 20;    /* default speed at edges     C*/

/* gesture related configuration constants for when wmode is enabled. */
static int   wmode_enabled	     = 1;     /* is wmode enabled        C */
static int   drag_lock_enabled	     = 1;     /* is drag locking enabled C */
static int   finger_threshold	     = 30;    /* pressure before it is a finger C */
static int   tap_lower_limit	     = 5;     /* a tap last at least this long C */
static int   tap_upper_limit	     = 200;   /* a tap is at most this long C */
static int   tap_range		     = 100;   /* mm finger movement limit  C */
static int   tap_interval	     = 200;   /* a tap reports button press this long C*/
static int   multiple_tap_delay      = 30;    /* time between reported button pressed C*/
static int   pads_tap_interval       = 8;     /* if pad sends gestures, what is it's tap_interval. C*/

/* wmode capabilities */
static int   palm_detect_enabled     = 1;     /* Ignore when palm on pad   C */
static int   palm_detect_level       = 12;    /* Detecting a palm level (between 0-11) C */
static int   multi_finger_tap_enabled= 1;     /* No of fingers decides which button is pressed C*/
static int   multi_finger_stop_enabled = 1;   /* less sensitive mouse with multi finger C*/
static int   multi_finger_stop_delay = 8;     /* how long to stop after a multifinger detection. C*/
static int   fake_finger_layer_enabled = 1;   /* add an extra software layer to detect multi fingers C*/


/* mixed configurations */
static int   touchpad_enabled        = 1; /* Disable the touch pad, see corner action turn_on_off. C*/
static int   stick_enabled           = 1; /* Some machines have a touchpad and a stick on
                                           * same device port. The stick will be ignored in
                                           * absolute mode, this option try to recognize
                                           * it's packets.                                         C*/
static int   stick_pressure_enabled  = 1; /* A (hard) press on the stick is reported as left click C*/
static int   four_way_button_enabled = 1; /* Round button giving 4 choices on some touchpads        C*/
static int   four_way_button_is_mouse= 1; /* Is the button: 4 buttons or does it moves the mouse.  C*/
static int   scrolling_enabled       = 1; /* Simulate wheel mouse in at the right edge             C*/
static int   scrolling_edge          = RIGHT_EDGE; /* Which edge is a the scrolling edge           C*/
static int   scrolling_speed         = 10;/* less is faster, 1 fastest  */
static float scrolling_button_factor = 0.5; /* How fast should a button/corner tap scroll, higher faster C*/
static int   auto_scrolling_enabled  = 1; /* Moving to the upper/lower edge keeps scrolling up/downC*/
static int   auto_scrolling_factor   = 2.0; /* How fast should autoscrolling be                   C*/
static int   reset_on_error_enabled  = 0; /* If a packet does not conform to any absolute protocol 
					   * should we reset the touchpad? This is wrong, because we
					   * should rather find out why it does that in first place.
					   * Do not turn it on per default.                         */


/*
** Types for describing actions.  When adding a new action, place it as
** the last item. It will break old configurefiles otherwise.
*/
typedef enum {
  No_Action          = 0,
  Left_Button_Action,
  Middle_Button_Action,
  Right_Button_Action,
  Fourth_Button_Action,
  Up_Button_Action,
  Down_Button_Action,
  Turn_On_Off_Action,
  Debug_On_Off_Action,
  Reset_Touchpad_Action,
  Toggle_Four_Way_Button_Action,
  Toggle_Stick_Pressure_Action,
  Toggle_Scrolling_Action,
  Left_Double_Click_Action,
} action_type;


typedef struct {
  int         action_mask;
  action_type action;
} touchpad_action_type;

static touchpad_action_type corner_actions [] = {
  { UPPER_LEFT_CORNER,  No_Action },
  { LOWER_LEFT_CORNER,  No_Action },
  { UPPER_RIGHT_CORNER, Middle_Button_Action },
  { LOWER_RIGHT_CORNER, Right_Button_Action  },
  { 0,                  No_Action } /* stop flag value */
};

static touchpad_action_type normal_button_actions [] = {
  { LEFT_BUTTON,        Left_Button_Action },
  { MIDDLE_BUTTON,      Middle_Button_Action },
  { RIGHT_BUTTON,       Right_Button_Action },
  { FOURTH_BUTTON,      Fourth_Button_Action },
  { UP_BUTTON,          Up_Button_Action },
  { DOWN_BUTTON,        Down_Button_Action },
  { 0,                  No_Action } /* stop flag value */
};

static touchpad_action_type multi_finger_actions [] = {
  { ONE_FINGER,         Left_Button_Action },
  { TWO_FINGERS,        Right_Button_Action },
  { THREE_FINGERS,      Middle_Button_Action },
  { 0,                  No_Action } /* stop flag value */
};

static touchpad_action_type four_button_actions [] = {
  { FOUR_LEFT_BUTTON,   Middle_Button_Action },
  { FOUR_RIGHT_BUTTON,  Fourth_Button_Action },
  { FOUR_UP_BUTTON,     Up_Button_Action },
  { FOUR_DOWN_BUTTON,   Down_Button_Action },
  { 0,                  No_Action } /* stop flag value */
};

static touchpad_action_type stick_actions [] = {
  { LEFT_BUTTON,        Left_Button_Action },
  { MIDDLE_BUTTON,      Middle_Button_Action },
  { RIGHT_BUTTON,       Right_Button_Action },
  { 0,                  No_Action } /* stop flag value */
};


/*
** These types are used to read the configuration data from the config file.
*/
typedef enum {
  Integer_Param,
  Float_Param,
  Flag_Param,
  Action_Param
} param_type_type;

typedef struct {
  char            *name;
  param_type_type  p_type;
  union {
    void  *gen_p;   /* avoids complaints by the compiler later on */
    int   *int_p;
    float *float_p;
    int   *flag_p;
    touchpad_action_type *corner_p;
  } addr;
} param_data_type;

static param_data_type param_data [] = {
  /* enabling configuration parameters */
  { "edge_motion_enabled",        Flag_Param,    {&edge_motion_enabled        }},
  { "edge_motion_speed_enabled",  Flag_Param,    {&edge_motion_speed_enabled  }},
  { "corner_taps_enabled",        Flag_Param,    {&corner_taps_enabled        }},
  { "tap_gesture_enabled",        Flag_Param,    {&tap_gesture_enabled        }},
  { "pressure_speed_enabled",     Flag_Param,    {&pressure_speed_enabled     }},
  { "tossing_enabled",            Flag_Param,    {&tossing_enabled            }},
  { "does_toss_use_static_speed", Flag_Param,    {&does_toss_use_static_speed }},
  { "tap_hold_edge_motion_enabled",Flag_Param,   {&tap_hold_edge_motion_enabled}},
  
  /* pressure induced speed related configuration parameters */		     
  { "low_pressure",               Integer_Param, {&low_speed_pressure         }},
  { "speed_up_pressure",          Integer_Param, {&speed_up_pressure          }},
  { "pressure_factor",            Float_Param,   {&speed_pressure_factor      }},
  { "standard_speed_factor",      Float_Param,   {&standard_speed_factor      }},
  /* toss/catch related parameters */		 			     
  { "min_toss_time",              Integer_Param, {&min_toss_time              }},
  { "max_toss_time",              Integer_Param, {&max_toss_time              }},
  { "prevent_toss_time",          Integer_Param, {&prevent_toss_time          }},
  { "min_toss_dist",              Integer_Param, {&min_toss_dist              }},
  { "static_toss_speed",          Integer_Param, {&static_toss_speed          }},
  { "toss_speed_factor",          Float_Param,   {&toss_speed_factor          }},
  /* edge motion related configuration parameters */			     
  { "x_min_center",               Integer_Param, {&x_min_center               }},
  { "x_max_center",               Integer_Param, {&x_max_center               }},
  { "y_min_center",               Integer_Param, {&y_min_center               }},
  { "y_max_center",               Integer_Param, {&y_max_center               }},
  { "edge_speed",                 Integer_Param, {&edge_speed                 }},
  /* use wmode */				 			     
  { "wmode_enabled",		  Flag_Param,	 {&wmode_enabled	      }},
  { "drag_lock_enabled",	  Flag_Param,	 {&drag_lock_enabled	      }},
  { "finger_threshold",		  Integer_Param, {&finger_threshold	      }},
  { "tap_lower_limit",		  Integer_Param, {&tap_lower_limit	      }},
  { "tap_upper_limit",		  Integer_Param, {&tap_upper_limit	      }},
  { "tap_range",		  Integer_Param, {&tap_range		      }},
  { "tap_interval",		  Integer_Param, {&tap_interval		      }},
  { "multiple_tap_delay",	  Integer_Param, {&multiple_tap_delay	      }},
  { "pads_tap_interval",	  Integer_Param, {&pads_tap_interval	      }},
  /* Additional wmode parameters */ 
  { "palm_detect_enabled",	  Flag_Param,	 {&palm_detect_enabled	      }},
  { "palm_detect_level",	  Integer_Param, {&palm_detect_level	      }},
  { "multi_finger_tap_enable",    Flag_Param,    {&multi_finger_tap_enabled   }},
  { "multi_finger_stop_enabled",  Flag_Param,    {&multi_finger_stop_enabled  }},
  { "multi_finger_stop_delay",    Integer_Param, {&multi_finger_stop_delay    }},
  { "fake_finger_layer_enabled",  Integer_Param, {&fake_finger_layer_enabled  }},
  /* Additional options*/
  { "touchpad_enabled", 	  Flag_Param,    {&touchpad_enabled	      }},
  { "stick_enabled",	          Flag_Param,    {&stick_enabled	      }},
  { "stick_pressure_enabled",     Flag_Param,    {&stick_pressure_enabled     }},
  { "four_way_button_enabled",    Flag_Param,    {&four_way_button_enabled    }},
  { "four_way_button_is_mouse",   Flag_Param,    {&four_way_button_is_mouse   }},
  { "scrolling_enabled",	  Flag_Param,    {&scrolling_enabled	      }},
  { "auto_scrolling_enabled",	  Flag_Param,    {&auto_scrolling_enabled     }},
  { "scrolling_edge",	          Integer_Param, {&scrolling_edge	      }},
  { "scrolling_speed",	          Integer_Param, {&scrolling_speed	      }},
  { "scrolling_button_factor",    Float_Param,   {&scrolling_button_factor    }},
  { "auto_scrolling_factor",      Float_Param,   {&auto_scrolling_factor      }},
  /* corner tap actions */			 			     
  { "upper_left_action",          Action_Param,  {&corner_actions [0]         }},
  { "lower_left_action",          Action_Param,  {&corner_actions [1]         }},
  { "upper_right_action",         Action_Param,  {&corner_actions [2]         }},
  { "lower_right_action",         Action_Param,  {&corner_actions [3]         }},
  /* no. of fingers tap actions */
  { "one_finger_tap_action",      Action_Param,  {&multi_finger_actions [0]   }},
  { "two_fingers_tap_action",     Action_Param,  {&multi_finger_actions [1]   }},
  { "three_fingers_tap_action",   Action_Param,  {&multi_finger_actions [2]   }},
  /* normal button actions */
  { "left_button_action",         Action_Param,  {&normal_button_actions [0]  }},
  { "middle_button_action",       Action_Param,  {&normal_button_actions [1]  }},
  { "right_button_action",        Action_Param,  {&normal_button_actions [2]  }},
  { "fourth_button_action",       Action_Param,  {&normal_button_actions [3]  }},
  { "up_button_action",           Action_Param,  {&normal_button_actions [4]  }},
  { "down_button_action",         Action_Param,  {&normal_button_actions [5]  }},
  /* 4 way button actions */
  { "four_way_left_button_action", Action_Param,  {&four_button_actions [0]   }},
  { "four_way_right_button_action",Action_Param,  {&four_button_actions [1]   }},
  { "four_way_up_button_action",   Action_Param,  {&four_button_actions [2]   }},
  { "four_way_down_button_action", Action_Param,  {&four_button_actions [3]   }},
  /* Synaptic Stick (passthrugh,stick) actions */
  { "stick_left_button_action",    Action_Param,  {&stick_actions [0]         }},
  { "stick_middle_button_action",  Action_Param,  {&stick_actions [1]         }},
  { "stick_right_button_action",   Action_Param,  {&stick_actions [2]         }},
  /* end of list */				 			     
  { NULL,                         Flag_Param,    {NULL                        }}
};


/*
** The information returned in the identification packet.
** STIG page 10
*/
typedef struct {
  int info_model_code;
  int info_major;
  int info_minor;
} info_type;

static info_type ident[2];

/*
** The information returned in the model ID packet.
** STIG page 11
*/

#define INFO_ROT180_BITS      0x800000   /* bit 23    */
#define INFO_PORTRAIT_BITS    0x400000	 /* bit 22    */
#define INFO_SENSOR_BITS      0x3F0000	 /* bit 16-21 */
#define INFO_HARDWARE_BITS    0x00FE00	 /* bit 9-15  */
#define INFO_NEW_ABS_BITS     0x000080	 /* bit 7     */
#define INFO_CAP_PEN_BITS     0x000040	 /* bit 6     */
#define INFO_SIMPLE_CMD_BITS  0x000020	 /* bit 5     */
#define INFO_GEOMETRY_BITS    0x00000F	 /* bit 0-3   */


typedef struct {
  int info_rot180; 
  int info_portrait; 
  int info_sensor; 
  int info_hardware; 
  int info_new_abs; 
  int info_cap_pen; 
  int info_simple_cmd; 
  int info_geometry; 
} model_id_type; 

static model_id_type model[2];

/*
** The sensor types as of STIG 2.5
** Page 11. Plus some guessing.
*/
static char *model_names [] = {
  "Unknown",                                /*  0 */
  "Standard TouchPad (TM41xx134)",
  "Mini Module (TM41xx156)",
  "Super Module (TM41xx180)",
  "Romulan Module",
  "Apple Module",
  "Single Chip",
  "Flexible pad (discontinued)",
  "Ultra-thin Module (TM41xx220)",
  "Wide pad Module (TW41xx230)",
  "TwinPad",                                /* 10 */
  "StampPad Module (TM41xx240)",
  "Sub-mini Module (TM41xx140)",
  "MultiSwitch module (TBD)",
  "Standard Thin",
  "Advanced Technology Pad (TM41xx221)",
  "Ultra-thin Module, connector reversed",
  "Low Power Module",
  "Thin Module, ATP",
  "Snap dome Module, ATP", 
  "FlexArm Module",                         /* 20 */
  "TWIII Module (TouchWriter 3)",
  "TWIII Module (P64)",
  "Combo Module",
  "Squish Module",
  "Thin TTL Serial module",
  "TWIII Ultra Thin Module",
  "PS/2 Passthrough", 
  "4 Button On Board Module",
  "6 Buttons Off Board Module",             
  "6 Buttons On Board Module",              /* 30 */
  "Ultrathin TTL serial Module",
  "ClearPad Module",
  "HyperThin Sensor Module",
  "Pad With Scroll Strip",
  "UltraThin TTL rounded, serial Module",
  "Ultrathin ATP Module",
  "SubMini w/6 buttons",
  "Standard USB Module",
  "cPad Dropin Plain USB Module",
  "cPad",                                   /* 40 */ 
  "",
  "",
  "",
  "UltraNav",
};


/*
** Define the information known about a sensor.
** STIG page 14.
** Resolution only apply for absolute mode.
** For older models the default resolution is 85x94.
*/
typedef struct {
  char *model;
  int   x_per_mm;
  int   y_per_mm;
  float width_mm;
  float height_mm;
} sensor_info_type;


static sensor_info_type sensor_info [] = {
  { "",            0,   0,  0.0,  0.0 },
  { "Standard",   85,  94, 47.1, 32.3 },
  { "Mini",       91, 124, 44.0, 24.5 },
  { "Super",      57,  58, 70.2, 52.4 },
  { "",            0,   0,  0.0,  0.0 },
  { "",            0,   0,  0.0,  0.0 },
  { "",            0,   0,  0.0,  0.0 },
  { "",            0,   0,  0.0,  0.0 },
  { "UltraThin",  85,  94, 47.1, 32.3 },
  { "Wide",       73,  96, 54.8, 31.7 },
  { "",            0,   0,  0.0,  0.0 },
  { "Stamp",     187, 170, 21.4, 17.9 },
  { "SubMini",   122, 167, 32.8, 18.2 },
};

static sensor_info_type *sensor[2] = {
  &sensor_info[0],&sensor_info[0]
};

/*
** The information returned in the extended capability packet.
** STIG page 15
*/

#define EXT_CAP_EXTENDED     0x8000     /* Bit 15  */
#define EXT_CAP_STICK        0x0080     /* Bit 8   */
#define EXT_CAP_PASS_THROUGH 0X0080     /* Bit 8!! */
#define EXT_CAP_SLEEP        0x0010     /* Bit 4   */
#define EXT_CAP_FOUR_BUTTON  0x0008     /* Bit 3   */
#define EXT_CAP_MULTI_FINGER 0X0002     /* Bit 1   */
#define EXT_CAP_PALM_DETECT  0X0001     /* Bit 0   */

typedef struct {
  int cap_ext; 
  int cap_stick;
  int cap_sleep; 
  int cap_four_button; 
  int cap_multi_finger; 
  int cap_palm_detect; 
} ext_cap_type;

static ext_cap_type capabilities[2];

/*
** The information in the mode byte.
** STIG Page 17
*/
#define ABSOLUTE_MODE      0x80      /* Bit 7 set */
#define RELATIVE_MODE      0x00	     /* Bit       */
#define HIGH_REPORT_RATE   0x40	     /* Bit 6 set = 80 packages per second */
#define LOW_REPORT_RATE    0x00	     /* Bit         40 packages per second */
#define USE_9600_BAUD      0x08	     /* Bit 3 for serial protocol */
#define USE_1200_BAUD      0x00	     /* Bit   */
#define PS2_SLEEP          0x08	     /* Bit 3 for ps2 protocol */
#define PS2_NO_SLEEP       0x00	     /* Bit   (only  button press activates touchpad again)*/
#define NO_TAPDRAG_GESTURE 0x04      /* Bit 2 for model version >= 4 */
#define TAPDRAG_GESTURE    0x00      /* Bit   */     
#define EXTENDED_REPORT    0x02	     /* Bit 1 for serial protocol absolute mode only */
#define NORMAL_REPORT      0x00	     /* Bit   */
#define STICK_DISABLE      0x02	     /* Bit 1 for ps2 protocol this disables any stick attached */
#define STICK_ENABLED      0x00	     /* Bit   */
#define REPORT_W_ON        0x01	     /* Bit 0 set */
#define REPORT_W_OFF       0x00      /* Bit   */



/*
** The format of the translated data to a report.
*/
typedef struct {
  int left;
  int middle;
  int right;
  int fourth;
  int up;
  int down;
  int x;
  int y;
  int pressure;
  int gesture;
  int fingers;
  int fingerwidth;
  int w;
} report_type;

static report_type last_report,cur_report;

/*
** A location record.
** This is needed to make an average over several packages, 
** because the reported x,y might not be that accurate.
*/
typedef struct {
  int x;
  int y;
} location_type;



/*
** Parameters for controlling the touchpad.
*/
/* touchpad information */
static int res_x;
static int res_y;
static int x_per_mm;
static int y_per_mm;


/* status information */
static int           packet_num = 0;
static int           was_edges = 0;
static int           was_non_edge = 0;
static location_type last_locs [4];
static Gpm_Event     last_state;
static int           tap_lower_limit_packet;    
static int           tap_upper_limit_packet;
static int           last_corner_action = GPM_B_NOT_SET;
static int           last_finger_action = GPM_B_NOT_SET;
static int           last_normal_button_actions[6] = 
  {GPM_B_NOT_SET,GPM_B_NOT_SET,GPM_B_NOT_SET,GPM_B_NOT_SET,GPM_B_NOT_SET,GPM_B_NOT_SET};
static int           last_stick_button_actions[3]  = 
  {GPM_B_NOT_SET,GPM_B_NOT_SET,GPM_B_NOT_SET};
static int           last_4_way_button_actions[4] =
  {GPM_B_NOT_SET,GPM_B_NOT_SET,GPM_B_NOT_SET,GPM_B_NOT_SET};

/* toss status information */
static int           is_tossing = 0;
static int           was_tossing = 0;
static int           min_toss_dist__2 = 32000;
static int           max_toss_packets;
static int           min_toss_packets;
static int           prevent_toss_packets;
static int           toss_timer;
static location_type toss_speed;
static location_type touch_loc;

/* Multi tap information */
static int           gesture_delay = 0;
static int           fake_forget_tap_interval = 0;  /* if hardware sends tap-hold, we need to keep track */
static int           fake_time_to_forget_tap = 0;   /* not to lose user defined actions in the hold periode. 
						     * (action like: Multifingers, non-repeating actions etc.*/

/* Multi finger information */
static int           was_fingers = 0;
static int           multi_finger_stop_timer = 0;
static int           multi_finger_pressure  = 0;
static int           multi_finger_xy = 0;


/* Scrolling information */
static int           is_scrolling = 0;            /* Scrolling using touchpad edge*/
static int           is_always_scrolling = 0;     /* Only report scrolling, no mouse movement */
static int           scrolling_speed_timer = 0;
static int           scrolling_amount_left = 0;   /* Tells how much to scroll up or down */





/****************************************************************************
**
** ROUTINES for processing either type of touchpad.
**
****************************************************************************/

/*
** Dump the report data for debugging.
**
** Because the synaptics sends (trivial) data in one second after last touch,
** which makes reading the debug data harder, only dump the report if it is different 
** than the previously dumped.
*/
static void tp_dump_report_data (report_type report,
				 int edges,
				 Gpm_Event* state) 
{
  static report_type  last_report_reported;
  static unsigned int times_report_repeated = 0;


  report.left   |= state->buttons & GPM_B_LEFT;
  report.right  |= state->buttons & GPM_B_RIGHT;
  report.middle |= state->buttons & GPM_B_MIDDLE;

  if (memcmp(&report,&last_report_reported,sizeof(report_type)) == 0){
    times_report_repeated++;
    return;
  }

  /* Was the last report repeated ? */
  if(times_report_repeated > 0){
    gpm_report (GPM_PR_DEBUG,"\rSynps2: Last report reported %d times\n",times_report_repeated); 
    times_report_repeated = 0;
  }

  last_report_reported = report;

  gpm_report (GPM_PR_DEBUG,
	      "\rSynps2: %c%c%c%c%c  %4dx%-4d %3d %2d %d  %c%c%c%c  %c%c    %3d%3d %d    %8d %8d %c",
	      report.fingers ? 'f' : '-',  
	      report.gesture ? 'g' : '-',
	      
	      report.left    ? 'l' : '-',
	      report.middle  ? 'm' : '-',  
	      report.right   ? 'r' : '-',
	      
	      report.x, report.y, report.pressure,
	      report.w,was_fingers, 
	      
	      edges & LEFT_EDGE   ? 'l' : '-',
	      edges & RIGHT_EDGE  ? 'r' : '-',
	      edges & BOTTOM_EDGE ? 'b' : '-',
	      edges & TOP_EDGE    ? 't' : '-',
	      
	      report.gesture && !report.fingers ? 't' : '-',
	      report.gesture && report.fingers  ? 'd' : '-',
	      
	      state->dx,state->dy,state->buttons,


	      multi_finger_pressure,multi_finger_xy,
	      (multi_finger_pressure>4500 && multi_finger_xy>50000? 'f':' '));

}


/* syn_dump_info
**
** Print properties for the hardare.
**
 */
static void syn_dump_info(int stick)
{

  gpm_report (GPM_PR_INFO,"Synaptic %s Device:",(stick?"Stick":"Touchpad"));
    
  gpm_report (GPM_PR_INFO,"Synaptics Ident:  model_code=%d   Firmware version %d.%d",
	      ident[stick].info_model_code, ident[stick].info_major, ident[stick].info_minor);
  gpm_report (GPM_PR_INFO,"Synaptics model:");
  gpm_report (GPM_PR_INFO,"   rot180:      %s", model[stick].info_rot180 ? "Yes" : "No");
  gpm_report (GPM_PR_INFO,"   portrait:    %s", model[stick].info_portrait ? "Yes" : "No");
  gpm_report (GPM_PR_INFO,"   sensor:      %d", model[stick].info_sensor);
  gpm_report (GPM_PR_INFO,"                %s", syn_model_name (model[stick].info_sensor));
  gpm_report (GPM_PR_INFO,"                %s", sensor[stick]->model);
  gpm_report (GPM_PR_INFO,"                %dx%d res/mm",
	      sensor[stick]->x_per_mm, sensor[stick]->y_per_mm);
  gpm_report (GPM_PR_INFO,"                %4.1fx%4.1f mm",
	      sensor[stick]->width_mm, sensor[stick]->height_mm);
  gpm_report (GPM_PR_INFO,"                %dx%d res", res_x, res_y);
  gpm_report (GPM_PR_INFO,"   hardware:    %d", model[stick].info_hardware);
  gpm_report (GPM_PR_INFO,"   newABS:      %s", model[stick].info_new_abs ? "Yes" : "No");
  gpm_report (GPM_PR_INFO,"   simpleCmd:   %s", model[stick].info_simple_cmd ? "Yes" : "No");
  gpm_report (GPM_PR_INFO,"   geometry:    %d", model[stick].info_geometry);
  gpm_report (GPM_PR_INFO,"   extended:    %s", capabilities[stick].cap_ext ? "Yes" : "No");
  gpm_report (GPM_PR_INFO,"   stick:       %s", capabilities[stick].cap_stick ? "Yes" : "No");
  gpm_report (GPM_PR_INFO,"   sleep:       %s", capabilities[stick].cap_sleep ? "Yes" : "No");
  gpm_report (GPM_PR_INFO,"   4 buttons:   %s", capabilities[stick].cap_four_button ? "Yes" : "No");
  gpm_report (GPM_PR_INFO,"   multifinger: %s", capabilities[stick].cap_multi_finger ? "Yes" : "No");
  gpm_report (GPM_PR_INFO,"   palmdetect:  %s", capabilities[stick].cap_palm_detect ? "Yes" : "No");

#if DEBUG_TOSS
  gpm_report (GPM_PR_INFO,"Min Toss Dist^2: %d\n", min_toss_dist__2);
#endif
}


/* Get model name, STIG page 11 */
static char *syn_model_name (int sensor) 
{
  if (sensor < 0 || 44 < sensor ) {
    return "Reserved";
  } else {
    return model_names [sensor];
  }
}

/* convert the model id from bits to values 
*  STIG page 11 
*/
static void syn_extract_model_id_info (int model_int, model_id_type *model) 
{
  model->info_rot180     = check_bits (model_int,  INFO_ROT180_BITS);
  model->info_portrait   = check_bits (model_int,  INFO_PORTRAIT_BITS);
  model->info_sensor     =            (model_int & INFO_SENSOR_BITS)   >> 16;
  model->info_hardware   =            (model_int & INFO_HARDWARE_BITS) >> 8;
  model->info_new_abs    = check_bits (model_int,  INFO_NEW_ABS_BITS);
  model->info_cap_pen    = check_bits (model_int,  INFO_CAP_PEN_BITS);
  model->info_simple_cmd = check_bits (model_int,  INFO_SIMPLE_CMD_BITS);
  model->info_geometry   =            (model_int & INFO_GEOMETRY_BITS);
}


/* Translate the reported data into a record for processing 
 *  STIG page 14*/
static sensor_info_type *syn_get_sensor_info (int sensor_id) 
{
  if (sensor_id < 0 || 12 < sensor_id ) {
    return &sensor_info [0];
  } else {
    return &sensor_info [sensor_id];
  }
}



/* Translate the reported data of extended capabilities STIG page 15.  If the
 extended bit is not set it should be assumed that neither of the other
 capabilities is available.*/
static void syn_extract_extended_capabilities(int ext_cap_int, ext_cap_type *cap)
{

# ifdef DEBUG  
  gpm_report(GPM_PR_INFO,"Synaptics Device Capabilities: %02X",ext_cap_int);
# endif 

  cap->cap_ext            = check_bits (ext_cap_int, EXT_CAP_EXTENDED);

  if(cap->cap_ext){
    cap->cap_stick        = check_bits (ext_cap_int, EXT_CAP_STICK);
    cap->cap_sleep        = check_bits (ext_cap_int, EXT_CAP_SLEEP);
    cap->cap_four_button  = check_bits (ext_cap_int, EXT_CAP_FOUR_BUTTON);
    cap->cap_multi_finger = check_bits (ext_cap_int, EXT_CAP_MULTI_FINGER);
    cap->cap_palm_detect  = check_bits (ext_cap_int, EXT_CAP_PALM_DETECT);
  }else{
    cap->cap_stick        = 0;
    cap->cap_sleep        = 0;
    cap->cap_four_button  = 0;
    cap->cap_multi_finger = 0;
    cap->cap_palm_detect  = 0;
    /* Wmode is not supported, but this should be turned off after reading the
       config file.*/
  }
}


/*
** Check for edges.
*/
static int tp_edges (location_type loc) 
{
  int edges = 0;
 
  if (loc.x <= x_min_center)
    edges |= LEFT_EDGE;

  if (loc.x >= x_max_center)
    edges |= RIGHT_EDGE;

  if (loc.y <= y_min_center)
    edges |= BOTTOM_EDGE;

  if (loc.y >= y_max_center)
    edges |= TOP_EDGE;

  return edges;
}


/**
 ** Handle scrolling. The wheel is way too fast to be usefull, so only report
 ** every scrolling_speed. This function is also called when scrolling is done by
 ** buttons, which is why DOWN and UP must be removed in the case of not scrolling.
 */

static void tp_handle_scrolling(Gpm_Event *state)
{
  /* Limit the amount of scrolling, so that we do not overrun. */
  if ( scrolling_amount_left >  256*20 )
    scrolling_amount_left = 256*20;

  if ( scrolling_amount_left < -256*20 )
    scrolling_amount_left = -256*20;

  state->buttons &= ~(GPM_B_DOWN | GPM_B_UP);
  
  if (scrolling_amount_left > scrolling_speed){
    scrolling_amount_left -= scrolling_speed;
    state->buttons |= GPM_B_UP;
  }else if (scrolling_amount_left < -scrolling_speed){
    scrolling_amount_left += scrolling_speed;
    state->buttons |= GPM_B_DOWN;
  } 
}


/*
** process_action
**
** Do the action and return a button state for a given action list and mask.
** Actions that should not be repeated should report GPM_B_NOT_SET (when
** holding down a button).
*/

static int tp_process_action(touchpad_action_type *action_list, int mask)
{
  int i = 0;
  int status = GPM_B_NOT_SET;
  static int Left_Double_Click = 0;

  if (mask == 0){
    gpm_report (GPM_PR_WARN,"Action Mask is 0");
    return GPM_B_NOT_SET;
  }
  
  while (action_list [i].action_mask) {
    if (check_bits (mask, action_list [i].action_mask)) {
      switch (action_list [i].action) {
      case Left_Button_Action:
	status = GPM_B_LEFT;
	break;
      case Middle_Button_Action:
	status = GPM_B_MIDDLE;
	break;
      case Right_Button_Action:
	status = GPM_B_RIGHT;
	break;
      case Fourth_Button_Action:
	status = GPM_B_FOURTH;
	break;
      case Up_Button_Action:
	scrolling_amount_left += ceil(scrolling_button_factor * scrolling_speed);
	status = GPM_B_UP;
	break;
      case Down_Button_Action:
	scrolling_amount_left -= ceil(scrolling_button_factor * scrolling_speed);
	status = GPM_B_DOWN;
	break;
      case Left_Double_Click_Action:
	Left_Double_Click++;
	status = GPM_B_LEFT; 
	if(Left_Double_Click == 2)
	  status = GPM_B_NONE; 
	if(Left_Double_Click == 4) {
	  status = GPM_B_NOT_SET; 
	  Left_Double_Click = 0;
	}
	break;
      case Turn_On_Off_Action:
	touchpad_enabled = !touchpad_enabled;
	status = GPM_B_NOT_SET;
	break;
      case Debug_On_Off_Action:
#       ifdef DEBUG_SYNAPTIC
	  debug_syn_to_stderr = !debug_syn_to_stderr;
#       endif
	status = GPM_B_NOT_SET;
	break;
      case Reset_Touchpad_Action:
	syn_ps2_reset(which_mouse->fd);
	syn_ps2_absolute_mode(which_mouse->fd);
	status = GPM_B_NOT_SET;
	break;
      case Toggle_Four_Way_Button_Action:
	four_way_button_is_mouse = !four_way_button_is_mouse;
	status = GPM_B_NOT_SET;
	break;
      case Toggle_Stick_Pressure_Action:
	stick_pressure_enabled = !stick_pressure_enabled;
	status = GPM_B_NOT_SET;
	break;
      case Toggle_Scrolling_Action:
	is_always_scrolling = !is_always_scrolling;
	status = GPM_B_NOT_SET;
	break;
      case No_Action:
	status = GPM_B_NOT_SET;
	break;
      default:
	gpm_report (GPM_PR_WARN,"Default Action: Action no. %X not defined",
		    action_list [i].action); 
	status = GPM_B_NOT_SET;
	break;
      }

      return status;
    }
    
    i++;
  }
  
  return status;
}


/*
** tp_process_corner_taps. 
**
** last_*_action is an easy way of remember which taps/buttons should call/repeat
** the action when the tap/button is held pressed. It could just as well  have
** been one variable where each bit held the repeater info for all the
** tap/buttons, but that will mean more code.
**
** The reason for not actually reusing last_*_action, instead of
** calculating it again, is that some actions like scrolling have "side effects".
** This may be perceived as a design fault.
**
** tp_process_corner_taps and tp_process_button_press are more complicated than
** to what is obvious. This is because 1) in a corner tap-and-hold the hold
** could take place at non edge. 2) A normal tap-and-hold can be moved to a
** corner.
*/
static void tp_process_corner_taps (Gpm_Event *state, report_type *report) 
{
  static int edges_at_cornertap_time;

  if (report->gesture && 
      ((is_corner (was_edges) && !last_report.gesture) || 
       (last_corner_action != GPM_B_NOT_SET))) {

    if(!last_report.gesture && last_corner_action == GPM_B_NOT_SET)
      edges_at_cornertap_time = was_edges;

    last_corner_action = tp_process_action(corner_actions,edges_at_cornertap_time);

    if (last_corner_action != GPM_B_NOT_SET)
      state->buttons |= last_corner_action;
  } 
}

/* tp_process_button_press
**
** Handles finger taps. Same way as tp_process_corner_taps.
**
** Should only calculate a tap if 
** 1) There is a gesture (tap)
** 2) The tap did not start at a corner with a corneraction
** 3) If it is calculated before it should have returned a repeating action.
*/
static void tp_process_finger_taps (Gpm_Event *state, report_type *report) 
{


  if (report->gesture && last_corner_action == GPM_B_NOT_SET &&
      !(corner_taps_enabled && is_corner (was_edges) && !last_report.gesture) &&
      !(last_report.gesture && last_finger_action == GPM_B_NOT_SET) ){

    if( ( multi_finger_tap_enabled && (was_fingers == 0 || was_fingers == 1)) ||
 	(!multi_finger_tap_enabled && was_fingers > 0))
      last_finger_action = tp_process_action(multi_finger_actions, ONE_FINGER);
    
    if (multi_finger_tap_enabled && was_fingers == 2)
      last_finger_action = tp_process_action(multi_finger_actions, TWO_FINGERS);
    
    if (multi_finger_tap_enabled && was_fingers == 3)
      last_finger_action = tp_process_action(multi_finger_actions, THREE_FINGERS);
    
    if (last_finger_action != GPM_B_NOT_SET) 
      state->buttons |= last_finger_action;
  }
}

/*
** tp_process_do_repeating_action
**
** The function test whether a feature's action should be repeated/called.  The
** action should be called if the feature is there and if it was there last
** time, then it should be a repeating action.
*/

static inline void 
tp_process_repeating_action(Gpm_Event *state, int feature, int last_feature, int *last_action, 
			    touchpad_action_type *action_list,int feature_mask)
{
  if (feature) {
    if (!(last_feature && *last_action == GPM_B_NOT_SET)) {
      *last_action = tp_process_action(action_list, feature_mask);
      if (*last_action != GPM_B_NOT_SET)
	state->buttons |= *last_action;
    }
  } else {
      *last_action = GPM_B_NOT_SET;
  }
}

/*
** tp_process_do_repeating_actions
**
** Call tp_process_do_repeating_action foreach possible action (right now only buttons.)
*/

static inline void 
tp_process_repeating_actions(Gpm_Event *state, int features, int last_features, int last_actions[], 
			     touchpad_action_type *action_list)
{
  int feature_no, feature_mask;

  for(feature_no = 0; feature_no < 8; feature_no++) {
    feature_mask = (1<<feature_no);
    tp_process_repeating_action(state, features & feature_mask, last_features & feature_mask,
				&last_actions[feature_no], action_list, feature_mask);
  }
}

/* tp_process_button_press
**
** Handles normal button presse explicitly because of the report
** layout (instead of using tp_process_do_repeating_actions).
*/
static void tp_process_button_press (Gpm_Event *state, report_type *report) 
{

  tp_process_repeating_action(state, report->left, last_report.left,
			      &last_normal_button_actions[0], normal_button_actions, LEFT_BUTTON);

  tp_process_repeating_action(state, report->right, last_report.right,
			      &last_normal_button_actions[1], normal_button_actions, RIGHT_BUTTON);

  tp_process_repeating_action(state, report->middle, last_report.middle,
			      &last_normal_button_actions[2], normal_button_actions, MIDDLE_BUTTON);

  tp_process_repeating_action(state, report->fourth, last_report.fourth,
			      &last_normal_button_actions[3], normal_button_actions, FOURTH_BUTTON);

  tp_process_repeating_action(state, report->up, last_report.up,
			      &last_normal_button_actions[4], normal_button_actions, UP_BUTTON);

  tp_process_repeating_action(state, report->down, last_report.down,
			      &last_normal_button_actions[5], normal_button_actions, DOWN_BUTTON);
}


/*
** syn_process_wmode_report
**
** Translate synaptics specific values.
*/


static void syn_process_wmode_report( report_type *report )
{
  /* STIG page 9: Values of w, vary from pad to pad. It is not precise when pressure is small < 25.
   * 4-7  finger of normal width
   * 8-14 very wide finger or palm
   * 15   maximum reportable width     
   */
  report->fingerwidth = max(0,report->w - 4);

  /* Check whether there is one finger on the pad */
  report->fingers  = (report->pressure > finger_threshold);
  
  /* use w values. w = 0: 2 fingers, w = 1: 3 fingers, (if there is pressure) */
  if (capabilities[0].cap_multi_finger){      
    if (report->pressure != 0 && (report->w == 0 || report->w == 1)){
      report->fingers = 2+report->w;
    }
  }
}



/*
** syn_ps2_process_extended_packets
**
** The internal synaptics ps2 touchpads can have extra devices attached (stick,
** 4 way button) to them, and they reports there state as special embedded
** packets. These attached devices are handled here.
**
** The function returns 1 if it does not make sense to continue normal touchpad
** processing, 0 otherwise.
**            
** I dont think that the external touchpads (serial) can have the same devices
** attached.  This code should be moved to ps2 specific part, but it is easy to
** have it here.
*/

static int syn_ps2_process_extended_packets( unsigned char *data, 
					     report_type *report,
					     Gpm_Event *state)
{
  static int last_stick_buttons = GPM_B_NONE;
  static int last_4_way_buttons = GPM_B_NONE;
  int tmp_buttons = GPM_B_NONE;

  /* Sanity check of data. */
  if ((report->pressure == 0 && (report->x != 0 || report->y != 0)) ||
      report->w == 3 ) {

     /* Something is wrong, should we assume it is an extended packet?  It
     *  cannot be processed further than here as the pressure is 0, which would
     *  break things, if the user uses it simultaneously with the touchpad. 
     */
    /* Allow some simultaneously usage: tap-hold on touchpad, with extended movement.
     * Do not do buttons, as they are not always correctly defined yet. */
    if (last_report.gesture) {
      report->gesture = 1;
      if(tap_gesture_enabled) tp_process_finger_taps (state, report);
      if(corner_taps_enabled) tp_process_corner_taps (state, report);
    }

    /* Stick invariant bits (I hope). See absolute packets */
    /* Stick pressed: The stick do only generates one packet, so double tap is
     * a problem; squeezing in a non-clicked state. It is probably not a real
     * problem, as it is hard not to move the stick a little between pressing,
     * thereby returning a non-clicked state between the packets. Maybe
     * press-lock mechanism is useful. Forget it, the styk supports it! */
    if((data[0] & 0xFC) == 0x84 && 
       (data[1] & 0xC8) == 0x08 &&
       (data[3] & 0xFC) == 0xC4){

      tp_process_button_press (state,report);

      if (stick_enabled) {
	state->dx=  ((data[1] & 0x10) ? data[4]-256 : data[4]);
	state->dy= -((data[1] & 0x20) ? data[5]-256 : data[5]);
      }

      if (stick_pressure_enabled) {
  	tmp_buttons  = ((data[1] & 0x01) ? STICK_LEFT_BUTTON   : 0);
  	tmp_buttons |= ((data[1] & 0x04) ? STICK_MIDDLE_BUTTON : 0);
  	tmp_buttons |= ((data[1] & 0x02) ? STICK_RIGHT_BUTTON  : 0);
	tp_process_repeating_actions(state,tmp_buttons,last_stick_buttons,
				     &last_stick_button_actions[0],stick_actions);
      } 

      last_stick_buttons = tmp_buttons;
            
#     ifdef DEBUG_STICK      
      gpm_report (GPM_PR_DEBUG,"StickData? %02x %02x %02x %02x %02x %02x :dx:%d dy:%d b:%d",
		  data[0],data[1],data[2],data[3],data[4],data[5],
		  state->dx,state->dy,state->buttons);
#     endif
      return 1;
    }

    /* 4 way button invariant bits (I hope). I dont know the official name. */
    if((data[0] & 0xFC) == 0x80 &&
       (data[1] & 0xFF) == 0x00 &&
       (data[3] & 0xFC) == 0xC0 &&
       report->x < 4 && report->y < 4 ){

      if(four_way_button_enabled){

	report->fourth = !report->fourth;   /* Note that this is reversed. */
	tp_process_button_press (state,report);

	if (four_way_button_is_mouse){
	  /* Report motion */
	  if (report->x & 1) /* UP */
	    state->dy = -1;
	  if (report->y & 1) /* DOWN */
	    state->dy = 1;
	  if (report->x & 2) /* LEFT */
	    state->dx = -1;
	  if (report->y & 2) /* RIGHT */
	    state->dx = 1;
	}else{ 
	  /* Report buttons */
	  tmp_buttons  = ((report->x & 1) ? FOUR_UP_BUTTON   : 0); /* UP */
	  tmp_buttons |= ((report->y & 1) ? FOUR_DOWN_BUTTON : 0); /* DOWN */
	  tmp_buttons |= ((report->x & 2) ? FOUR_LEFT_BUTTON : 0); /* LEFT */
	  tmp_buttons |= ((report->y & 2) ? FOUR_RIGHT_BUTTON: 0); /* RIGHT */
	  tp_process_repeating_actions(state,tmp_buttons,last_4_way_buttons,
				       &last_4_way_button_actions[0],four_button_actions);	  
	}
      }

      last_4_way_buttons = tmp_buttons;

      if ( scrolling_amount_left != 0 ){
	tp_handle_scrolling(state);
      }

      return 1;
    } 

    /* This is unknown packet. */
    gpm_report (GPM_PR_ERR,"\rSynps2: Pressure is 0, but x or y is not 0. "
		           "Data: %02X %02X %02X %02X %02X %02X",
		data [0],data [1],data [2],data [3],data [4],data [5]);
    return 1;
  }

  /* Multiplexing with the stick (guest) device. */
  state->buttons |= last_4_way_buttons | last_stick_buttons;

  return 0;
}

/**
** Preprocess the report even before doing wmode stuff.
** Is always called directly after the conversion to check the data received.
** return 0 if it is reasonable, 1 if there is something wrong.
**
** Checks for correct data package, palm on the pad, number of fingers.
**            
*/

static int tp_find_fingers ( report_type *report,
			     Gpm_Event *state)
{

  static int fake_extra_finger;
  static int was_fake_pressure;

  /* Check whether there is a palm on the pad */
  if (palm_detect_enabled &&  
      report->fingers && (report->fingerwidth >= palm_detect_level)){
#   ifdef DEBUG_PALM
    gpm_report (GPM_PR_DEBUG,"\rTouchpad: palm detected. finger width: %d",report->fingerwidth); 
#   endif
    /* BUG should not return 1, as this drops packets. Return a repeated report ? */
    /*       last_locs [mod4 (packet_num - 1)].x = report->x; */
    /*       last_locs [mod4 (packet_num - 1)].y = report->y; */
    /*     or */
    /*       report->pressure = 0; */
    return 1;
  }


  /* Extra check for vertical multi fingers which my pad is very bad to detect.
  ** Only check for extra fingers if no of fingers has not changed.  Faking
  ** fingers may go wrong so sanity check needed.  This is not an attempt to
  ** know the number of fingers all the time, as this is not needed. 
  */
  if (fake_finger_layer_enabled){
    
    if (report->fingers > 1){
      fake_extra_finger = 0;
    }
        
    if(report->fingers == 0){
      fake_extra_finger = 0;
#     ifdef DEBUG_REPORTS
        multi_finger_pressure  = 0;
        multi_finger_xy = 0;
#     endif
    }else
      if (report->fingers + fake_extra_finger == last_report.fingers) {

	multi_finger_pressure = sqr(report->pressure) - sqr(last_report.pressure);
	multi_finger_xy = (sqr(last_locs [mod4 (packet_num - 1)].x - report->x) +
			   sqr(last_locs [mod4 (packet_num - 1)].y - report->y) );

	/* Check for a second finger. If the move is larger than tap range,
	* then it is not a tap, and adding the finger does not change reported        
	* buttons, but it will still stop the moving if
	* multi_finger_stop_enabled is on. These tests are complete base on my
	* imagination and experience, so any better idea are welcome. */
	if(report->fingers == 1 && 
	   multi_finger_pressure > 4500 && multi_finger_xy > 2*sqr((double)tap_range)){
	    fake_extra_finger = 1;
	    was_fake_pressure =  report->pressure;

	}/* Check for third finger. */
	else if(last_report.pressure  > 180  && (report->fingers + fake_extra_finger) == 2 &&
		 multi_finger_pressure > 4500 && multi_finger_xy > 2*sqr((double)tap_range)){
	  fake_extra_finger = 2;
	  was_fake_pressure = report->pressure;

	} else if ( (fake_extra_finger > 0) &&
		    (was_fake_pressure - report->pressure > 20)){
	  fake_extra_finger --;

	} else if(multi_finger_pressure < -5000 && multi_finger_xy > 2*sqr((double)tap_range)){
	  /* Probably missed a placed multi finger, as this is one removed! */
	  last_report.fingers ++;
	}
      }
    
    report->fingers += fake_extra_finger;
    
  }


  /* Check whether to reduce sensibility when adding or removing fingers */
  if (multi_finger_stop_enabled){

    /* Is a finger added or removed since last packet? */
    if( (report->fingers     > 1 && report->fingers > last_report.fingers) ||
	(last_report.fingers > 1 && report->fingers < last_report.fingers) ){

      /* Updating the timer right after another added/removed finger,
       * would make the undo moving become redo, so dont.*/
      if(multi_finger_stop_timer != multi_finger_stop_delay - 1)
	multi_finger_stop_timer  = multi_finger_stop_delay;

#     ifdef DEBUG_MULTI_FINGER
      gpm_report (GPM_PR_DEBUG,"%s multi finger %d %d %d",
		  report->fingers > last_report.fingers ? "Add":"Remove",
		  last_report.fingers,report->fingers,fake_extra_finger);
#     endif
      
    } /* Should be tested last, because of undo moving when removing fingers. */
    else if(report->fingers == 0){
	multi_finger_stop_timer = 0;
    }

    /* Stop moving the mouse after a finger adding/removing. */
    if( multi_finger_stop_timer > 0){
      last_locs [mod4 (packet_num - 1)].x = report->x;
      last_locs [mod4 (packet_num - 1)].y = report->y;
      last_locs [mod4 (packet_num - 2)].x = report->x;
      last_locs [mod4 (packet_num - 2)].y = report->y;

      /* Undo the previous move (before detecting of the adding/removing). */
      if (multi_finger_stop_timer == multi_finger_stop_delay){
	last_locs [mod4 (packet_num - 2)].x += last_state.dx * 2 / standard_speed_factor;
	last_locs [mod4 (packet_num - 2)].y -= last_state.dy * 2 / standard_speed_factor;	
      }

      multi_finger_stop_timer --;
    }
  }

  /* If wmode is not used, we do not know all the informations to reset the
   * was_fingers variable or stop waiting for a tap-hold, so set a timer to
   * reset the variables when we believe it is time.  pads_tap_interval is
   * optimal when set to the touchpads tap_interval.
   */

  if(fake_forget_tap_interval){
    if (report->fingers || report->gesture)
      fake_time_to_forget_tap = pads_tap_interval;

    if(fake_time_to_forget_tap > 0)
      fake_time_to_forget_tap --;
    else
      was_fingers = 0;
  }


  was_fingers = max(was_fingers,report->fingers);

  return 0;
}


/** 
** tp_find_gestures
** Process the report from a wmode enabled device.  No gesture calculation is
** done by the device in wmode, so the find tap and drag hold and tap hold gestures.
**
** Tap mechanism: High when touched. (=: assigned, -: decreased, +: increased)
**
**                         _________________________  
**                         | tap_lower/upper_limit |tap_interval|  
**         -----------------                       -----------------
** 
** stroke_x/y         :    =                         
** finger_on_pad_timer: 0000123        +++++++++++++0000000000000000
** time_to_forget_tap : 0000000        000000000000=---------3210000
** gesture_delay      : 0000000  ...   00000000000000000000000000000
** drag_locked        : 0000000        00000000000000000000000000000
** gesture            : 0000000        00000000000011111111111110000
**
** Note:
**  0) When gesture is high a button is reported pressed (usually left button).
**  1) finger_on_pad_timer is increased at most to 1 larger than tap_upper_limit,
**     but then it is not a tap any longer.
**  2) If tap_interval is larger than 1s (80 packets) then time_to_forget_tap never reaches 0,
**     before the touchpads stop sending packets, so gesture is reported until touched again.  
**
**
** Tap hold mechanism: High when touched. (=: assigned, -: decreased, +: increased)
**
**                                       tap_interval
**                         ________________  v ___   ___
**                         |tap_l/u_limit |    |not tap|
**         -----------------              ------       --------    
** 
** stroke_x/y         :    =                   =      
** finger_on_pad_timer: 0000123+       +++00000123++  +++0000
** time_to_forget_tap : 00000000       000=----11111  1110000
** gesture_delay      : 00000000 ...   0000000000000  0000000
** drag_locked        : 00000000       0000000000000  0000000
** gesture            : 00000000       0001111111111  1110000
**
** Note:
**  0) time_to_forget_tap is set to 1 when touching the pad 2 time, so the gesture is stop when released. 
**     (two fast taps would be a problem).
**
**
** Two consecutive taps mechanism: High when touched. (=: assigned, -: decreased, +: increased)
**
**                                       tap_interval  multiple_tap_delay
**                         ________________  v _______________  v   |
**                         |tap_l/u_limit |    |tap_l/u_limit|  
**         -----------------              ------             --------    
** 
** stroke_x/y         :    =                   =      
** finger_on_pad_timer: 0000123+       +++00000123++       ++00000000000000000
** time_to_forget_tap : 00000000       000=----11111       11=---------32100000
** gesture_delay      : 00000000 ...   0000000000000 ...   00=----3210000000000
** drag_locked        : 00000000       0000000000000       00000000000000000000
** gesture            : 00000000       0001111111111       11000000001111100000
**
** Note:
**  0) There is no gesture if gesture_delay is non zero.
**  1) multiple_tap_delay (gesture_delay) should be less than tap_interval (time_to_forget_tap),
**     or else no second tap is reported.
**  2) Three fast taps (shorter than multiple_tap_delay) would reset gesture_delay the second time 
**     and only report 2 taps (the first and last). (likewise with more fast taps). BUG ?
**
**
** Drag lock mechanism: High when touched. (=: assigned, -: decreased, +: increased)
** MISSING
*/


static void tp_find_gestures (report_type *report) 
{
  static int finger_on_pad_timer = 0;
  static int time_to_forget_tap = 0;
  static int stroke_x;
  static int stroke_y;
  static int drag_locked = 0;


  if (report->fingers > 0) {
	    
    /* finger down for the first time */
    if (finger_on_pad_timer == 0) { 
      stroke_x = report->x;
      stroke_y = report->y;
    }
    
    /* don't want timer to overflow */
    if (finger_on_pad_timer < (tap_upper_limit_packet)) 
      finger_on_pad_timer ++; 
    
    /* dragging and consecutive tap gestures is to end with finger up 
     *  forget fast that there was a tap if this is not a part of a tap.*/
    if (time_to_forget_tap > 0) 
      time_to_forget_tap = 1;
    
  } else { /* interesting things happen when finger is up */
      
    /* tap determination: Was the finger long enough on the pad and not too
     * long, while staying at the same place.
     */
    if ((finger_on_pad_timer > (tap_lower_limit_packet)) &&  /* minimum finger down time */
	(finger_on_pad_timer < (tap_upper_limit_packet)) &&  /* maximum finger down time */
	((distance((double)(stroke_x - last_report.x),  /* maximum range for finger to drift while down */
		   (double)(stroke_y - last_report.y))
	 < sqr((double)tap_range)) ||
	 (multi_finger_tap_enabled && was_fingers > 1))) {
      
      /* not a consecutive tap? */
      if (time_to_forget_tap == 0) 
	gesture_delay = 0; /* right -> don't delay gesture */
      else { /* a consecutive tap! */
	gesture_delay = multiple_tap_delay * 80 / 1000; /* delay gesture to create multiple click */
      }
      
      /* is drag locked */
      if (drag_locked) {
	drag_locked = 0; /* unlock it and don't gesture. */
	time_to_forget_tap = 0;
      } else 
	time_to_forget_tap = tap_interval * 80 / 1000; /* setup gesture time to count down */
      
    } else { /* It was not a tap */
      
      /* a drag to lock?  If user did a tap and quickly hold the finger longer than a tap.
       */
      if (drag_lock_enabled && 
	  (time_to_forget_tap > 0) && (finger_on_pad_timer >= (tap_upper_limit_packet)))
	drag_locked = 1;
      
      if (time_to_forget_tap  > 0) time_to_forget_tap --;
      if (time_to_forget_tap == 0) was_fingers=0;
      if (gesture_delay > 0) gesture_delay --;
      
    }
    
#   ifdef DEBUG_TAPS
    if (finger_on_pad_timer)
      gpm_report (GPM_PR_DEBUG,"A tap? %d < %d < %d && (%d)^2 + (%d)^2 < %d",
		  tap_lower_limit_packet,finger_on_pad_timer,tap_upper_limit_packet,
		  stroke_x-last_report.x,stroke_y-last_report.y,tap_range*tap_range);
#   endif

    finger_on_pad_timer = 0;
    
  }
  
  report->gesture  = ((time_to_forget_tap > 0) && (gesture_delay == 0)) || drag_locked;
}


/*
** tp_process_report
**
** Process the touchpad report. Do tossing mechanism, edge mechanism (edge
** extension, corner taps), speed pressure.
**
** Tossing mechanism: High when touched.  (=: assigned, -: decreased)
**                                                                         (Pause)
**                 ________________________        _____         ______ prevent_toss ___
**                 | min/max_toss_packets |  < 1s  |      ....        |      ...     |  
** -----------------                      ----------                  -----       ----  
** 
** is_tossing : 000000000000000000000000001111111111000000000000000000000000000000000000 
** was_tossing: 000000000000000000000000000000000001111111111111111111111111111111100000
** toss_timer :    =======================         ====================---- ... 321000==  
**                           
** Note 
** 0) When is_tossing is high a motion is reported/calculated.
** 1) if prevent_toss delay is 0 then toss to toss is allowed without pause.
** 2) if the pause is to short (< prevent_toss), the timers start over!
** 3) The 1 second limit is due to the touchpad only sends packets one second after a release.
** 
**
*/
static void tp_process_report (Gpm_Event *state,
			       report_type *report) 
{
  location_type loc;
  int           edges;
  float         pressure_speed_factor;
  float         edge_speed_factor;
  int           edge_motion_on;
  float         dx, dy;

  /* extract location and edges */
  loc.x = report->x;
  loc.y = report->y;
  edges = tp_edges (loc);


  if (report->fingers > 0) {

    if (tossing_enabled) {
      /* this is the cue to stop tossing, if we are tossing so
         Force a touch free pause before a new tossing is allowed (prevent toss time) */
      was_tossing = (was_tossing || is_tossing);
      toss_timer  = prevent_toss_packets;
      is_tossing  = 0;
      
      /* if we start tossing then this is from where */
      if (last_report.fingers == 0) {
	touch_loc = loc;
      }
    }

    /* Save the edge state, so we can detect a movement from non edge to edge. */
    if (!edges)
      was_non_edge = 1;

    /* Need enough packets to perform smoothing on dy and dx
    *  but it should work with only 2, so why is 4 required? */
    if (packet_num > 3) {

      /* Is this start of scrolling? */
      if (scrolling_enabled && !is_scrolling)
	is_scrolling  = ((edges & scrolling_edge)  && /* Note: only one &! */
			 !was_non_edge         && /* Must start at edge */
			 !last_report.gesture  && /* No consecutive tap */
			 (!corner_taps_enabled || /* Corner disabled or */
			  !is_corner (edges)));   /* no corner          */


      /* 
      ** 1) if edge motion is enabled, only activate if we moved into the edge or
      ** if not using the corners taps.  Dont activate right away after a gesture
      ** from a corner tap. 2) No harm is done if activated when scrolling (as
      ** long as the scrolling edge is vertical). 3)If user is tap holding (then
      ** he cannot lift the finger and hit the touchpad boarder).
      */

      edge_motion_on  = (edges && edge_motion_enabled &&
			  (was_non_edge         || 
			   !corner_taps_enabled ||
			  (!is_corner(edges) && !last_report.gesture)));
      edge_motion_on |= (edges && auto_scrolling_enabled && is_scrolling);
      edge_motion_on |= (edges && tap_hold_edge_motion_enabled && last_report.gesture && 
			 was_non_edge);


      /* compute the speed factor based on pressure */
      pressure_speed_factor = standard_speed_factor;

      if (report->pressure > speed_up_pressure) {
	pressure_speed_factor *= 1.0 + ((report->pressure - speed_up_pressure) *
			       speed_pressure_factor);
      }

      /* use the edge speed factor if edge_motion_speed is enabled */
      edge_speed_factor = (edge_motion_speed_enabled ?
			   pressure_speed_factor :
			   standard_speed_factor);
      
      pressure_speed_factor = (pressure_speed_enabled ?
			       pressure_speed_factor :
			       standard_speed_factor);
      
      if ( auto_scrolling_enabled && is_scrolling ){
	edge_speed_factor *= 1.0 + auto_scrolling_factor; 
      }


      /* Calculate dx and dy depending on whether we are at an edge */

      if (edge_motion_on && (edges & TOP_EDGE))
	 state->dy = -edge_speed * edge_speed_factor;
      else if (edge_motion_on && (edges & BOTTOM_EDGE))
	 state->dy = edge_speed * edge_speed_factor;
      else
	 state->dy = (pressure_speed_factor *
		      (((last_locs [mod4 (packet_num - 1)].y +
			 last_locs [mod4 (packet_num - 2)].y) / 2) - 
		       ((loc.y +
			 last_locs [mod4 (packet_num - 1)].y) / 2)));
      
      if (edge_motion_on && (edges & LEFT_EDGE))
	 state->dx = -edge_speed * edge_speed_factor;
      else if (edge_motion_on && (edges & RIGHT_EDGE))
	 state->dx = edge_speed * edge_speed_factor;
      else
	 state->dx = (pressure_speed_factor *
		      (((loc.x +
			 last_locs [mod4 (packet_num - 1)].x) / 2) - 
		       ((last_locs [mod4 (packet_num - 1)].x +
			 last_locs [mod4 (packet_num - 2)].x) / 2)));
    } /* If (packet_num > 3)*/

    last_locs [mod4 (packet_num)] = loc;
    was_edges = edges;
    packet_num++;
  } else {
    /* No finger on the pad */
    /* Start the tossing action if enabled */
    if (tossing_enabled && was_fingers == 1 &&
	!was_tossing    &&
	!is_scrolling   &&
	(packet_num > min_toss_packets) &&
	(packet_num < max_toss_packets)) {

      dx =    last_locs [mod4 (packet_num - 1)].x - touch_loc.x;
      dy = - (last_locs [mod4 (packet_num - 1)].y - touch_loc.y);

#if DEBUG_TOSS
      gpm_report (GPM_PR_INFO,"dx: %2.1f  dy: %2.1f  tdist^2: %2.1f",
		  dx, dy, distance (dx, dy));
#endif

      if (distance (dx, dy) > min_toss_dist__2) {
	is_tossing = 1;

	/* determine the toss speed */
	if (does_toss_use_static_speed) {
	  toss_speed.x = (static_toss_speed * standard_speed_factor *
			  dx / (abs (dx) + abs (dy)));
	  toss_speed.y = (static_toss_speed * standard_speed_factor *
			  dy / (abs (dx) + abs (dy)));
	} else {
	  toss_speed.x = dx * standard_speed_factor * toss_speed_factor;
	  toss_speed.y = dy * standard_speed_factor * toss_speed_factor;
	}

#if DEBUG_TOSS
	gpm_report (GPM_PR_INFO,"tossx: %d  tossy: %d", toss_speed.x, toss_speed.y);
#endif
      }
    }

    /* no fingers therefore: no edge, and restart packet count */
    was_non_edge = 0;
    is_scrolling = 0;
    scrolling_speed_timer = 0;
    packet_num = 0;
  }


  /* if we are tossing then apply the toss speed */
  if (tossing_enabled && is_tossing) {
    state->dx = toss_speed.x;
    state->dy = toss_speed.y;
    }


  /* if we are scrolling then stop moving and report wheel amount.  The reason
  ** for having this above buttons actions is buttons can then move the
  ** mouse while scrolling. 
  */
  if ((scrolling_enabled && is_scrolling) ||
      is_always_scrolling){
    scrolling_amount_left -= state->dy;
    state->dx = 0;
    state->dy = 0;
  }


  /* check for (corner)buttons if we didn't just complete a toss or is scrolling */
  if (!is_tossing && !was_tossing && !is_scrolling) {
    /*
    ** If there is no gesture then there are no buttons, but dont clear if we
    ** are about to make a double tap.  Otherwise compute new buttons.
    */
    if (!report->gesture && 
	!report->left && !report->right && !report->middle && !report->fourth ) {
      if (!gesture_delay && !fake_time_to_forget_tap) {
	last_corner_action = GPM_B_NOT_SET;
	last_finger_action = GPM_B_NOT_SET;
      }
      state->buttons = GPM_B_NONE;
    } else {
      tp_process_button_press (state, report);
      if(tap_gesture_enabled) tp_process_finger_taps (state, report);
      if(corner_taps_enabled) tp_process_corner_taps (state, report);
    }
  }

  /* Is there any amount of scrolling left?  Should be checked after corner actions. */
  if (scrolling_amount_left != 0){
    tp_handle_scrolling(state);
  }


  was_tossing  = was_tossing && toss_timer;
  if (was_tossing)
    toss_timer--;


  /* remember the last state of the finger for toss processing */
  last_report   = *report;
  last_state    = *state;

  /* Dont do anything if the pad is not enabled, but after corner actions are
   * done so it can be turn on again. */
  if (!touchpad_enabled){
    state->buttons = GPM_B_NONE;
    state->dx      = 0;
    state->dy      = 0;
  }

#if DEBUG_REPORTS
  tp_dump_report_data (*report, edges, state);
#endif

}


/*
** syn_read_config_file
**
** Read the configuration data from the global config file
** SYSCONFDIR "/gpm-syn.conf".
*/
void tp_read_config_file (char* config_filename) 
{
  char line [80];
  char *token;
  char *end_ptr;
  int  param, tmp_read_int_param;
  FILE *config;
  char full_filename[100];
  int  status;
  float tmp_read_float_param;

  status = snprintf(full_filename,100,SYSCONFDIR "/%s",config_filename);
  if (status < 0) {
    gpm_report (GPM_PR_WARN,"Too long path for configure file: %s", config_filename); 
    return;
  }


  if ( !(config = fopen (full_filename, "r")) ) {
    gpm_report (GPM_PR_WARN,"Failed to open configfile: %s", full_filename);    
  }else{
    while (fgets (line, 80, config)) {
      if (line [0] == '[') {
	if ( (token = strtok (line, "[] \t")) ) {
	  param = 0;

	  /* which param is it */
	  while (param_data [param].name &&
		 strcasecmp (token, param_data [param].name) != 0) {
	    param++;
	  }

	  /* was a param found? */
	  if (!param_data [param].name) {
	    gpm_report (GPM_PR_WARN,"Unknown parameter %s", token);
	  } else {
	    token = strtok (NULL, "[] \t");

	    switch (param_data [param].p_type) {
	    case Integer_Param:
	      tmp_read_int_param = strtol (token, &end_ptr, 0);
	      if (end_ptr != token)
		*(param_data [param].addr.int_p) = tmp_read_int_param;
	      else
		gpm_report (GPM_PR_WARN,"Integer value (%s) for parameter %s is invalid",
			    token, param_data [param].name);
#             if DEBUG_PARAMS
	      gpm_report (GPM_PR_INFO,"Param %s set to %d",
			  param_data [param].name,
			  *(param_data [param].addr.int_p));
#             endif
	      break;

	    case Float_Param:
	      tmp_read_float_param = strtod (token, &end_ptr);
	      if (end_ptr != token)
		*(param_data [param].addr.float_p) = tmp_read_float_param;
	      else
		gpm_report (GPM_PR_WARN,"Float value (%s) for parameter %s is invalid",
			    token, param_data [param].name);
#             if DEBUG_PARAMS
	      gpm_report (GPM_PR_INFO,"Param %s set to %3.3f",
			  param_data [param].name,
			  *(param_data [param].addr.float_p));
#             endif
	      break;

	    case Flag_Param:
	      if (index ("YyTt1", token [0])) {
		*(param_data [param].addr.flag_p) = 1;
	      } else if (index ("NnFf0", token [0])) {
		*(param_data [param].addr.flag_p) = 0;
	      } else {
		gpm_report (GPM_PR_WARN,"Flag value (%s) for parameter %s is invalid",
			    token, param_data [param].name);
	      }
#             if DEBUG_PARAMS
	      gpm_report (GPM_PR_INFO,"Param %s set to %s",
			  param_data [param].name,
			  (*(param_data [param].addr.flag_p) ?
			   "True" : "False"));
#             endif
	      break;

	    case Action_Param:
	      tmp_read_int_param = strtol (token, &end_ptr, 0);
	      if (end_ptr != token)
		param_data [param].addr.corner_p->action = tmp_read_int_param;
	      else
		gpm_report (GPM_PR_WARN,"Action value (%s) for parameter %s is invalid",
			    token, param_data [param].name);
#             if DEBUG_PARAMS
	      gpm_report (GPM_PR_INFO,"Param %s set to %d",
			  param_data [param].name,
			  param_data [param].addr.corner_p->action);
#             endif
	      break;

	    default: ;
	    }
	  }
	}
      }
    }

    fclose (config);
  }
}


/*
** syn_process_touchpad_config
**
** Extract important information and report (as desired) to the user.
*/
static void syn_process_config (info_type ident,
				model_id_type model) 
{
  sensor[0] = syn_get_sensor_info (model.info_sensor);
  gpm_report (GPM_PR_INFO, "     Firmware version %d.%d\n",
	      ident.info_major, ident.info_minor);

  tp_read_config_file ("gpm-syn.conf");

  
  /* Limit the options depending on the touchpad capabilities. This should be
     done after reading the configure file so they may be turned off on purpose
     and can'nt be turned on if not supported. */
  if(!capabilities[0].cap_ext){
    wmode_enabled         = 0;
  }

  if(!wmode_enabled || !capabilities[0].cap_palm_detect){
    palm_detect_enabled   = 0;
  }

  if(!wmode_enabled || !capabilities[0].cap_stick){
    stick_enabled          = 0;
    stick_pressure_enabled = 0;
  }
  
  /* fake_forget_tap_interval must be set if the hardware does the gesture. */
  if(!wmode_enabled)
    fake_forget_tap_interval = 1;

  /* Save important information */
  x_per_mm = sensor[0]->x_per_mm;
  y_per_mm = sensor[0]->y_per_mm;
  res_x = (int) (x_per_mm * sensor[0]->width_mm);
  res_y = (int) (y_per_mm * sensor[0]->height_mm);

  /* convert the tap times to packets (80 pkts/sec and 1000 ms/sec) */
  tap_lower_limit_packet = tap_lower_limit * 80 / 1000;
  tap_upper_limit_packet = tap_upper_limit * 80 / 1000;

  /* Convert the toss dist to touchpad resolution from mm */
  min_toss_dist__2 = sqr (min_toss_dist * (x_per_mm + y_per_mm) / 2);

  /* convert the toss times to packets (80 pkts/sec and 1000 ms/sec) */
  max_toss_packets     = max_toss_time * 80 / 1000;
  min_toss_packets     = min_toss_time * 80 / 1000;
  prevent_toss_packets = prevent_toss_time * 80 / 1000;

}


/****************************************************************************
**
** ROUTINES for interfacing to a SERIAL touchpad
**
****************************************************************************/




static unsigned char tp_hextoint (unsigned char byte1,
				  unsigned char byte2) 
{
  unsigned char bytes [3];
  int result;

  bytes [0] = byte1;
  bytes [1] = byte2;
  bytes [2] = '\0';
  sscanf (bytes, "%x", &result);
  return result;
}

static void tp_serial_flush_input (int fd) 
{
  struct timeval tv;
  fd_set rfds;
  unsigned char junk;
  
  FD_ZERO(&rfds);
  FD_SET (fd, &rfds);
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  usleep (50000);

  while (select (fd+1, &rfds, NULL, NULL, &tv) == 1) {
#if DEBUG_FLUSH
    gpm_report (GPM_PR_INFO,"Serial tossing");
    fflush (stdout);
#endif
    read (fd, &junk, 1);
#if DEBUG_FLUSH
    gpm_report (GPM_PR_INFO," %c", junk);
#endif
    FD_ZERO(&rfds);
    FD_SET (fd, &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;
  }
}

static void tp_serial_read (int fd,
			    unsigned char *bytes,
			    size_t count) 
{
  struct timeval tv;
  fd_set rfds;
  int num_read = 0;
  int read_count;

  FD_ZERO (&rfds);
  FD_SET (fd, &rfds);
  tv.tv_sec = 1;
  tv.tv_usec = 0;

  while ((select (fd+1, &rfds, NULL, NULL, &tv) == 1) &&
	 (num_read < count)) {
    read_count = read (fd, &bytes [num_read], count - num_read);
    num_read += read_count;

    FD_ZERO (&rfds);
    FD_SET (fd, &rfds);
  }

  for (; num_read < count; num_read++) {
     bytes [num_read] = '\0';
  }
}

/* Write a string of commands */
static void tp_serial_send_cmd(int fd,
			       unsigned char *cmd) 
{
  unsigned char junk [15];

  tp_serial_flush_input (fd);
  write (fd, cmd, strlen (cmd));
  tp_serial_read (fd, junk, strlen (cmd));
#if DEBUG_FLUSH
  junk [strlen (cmd)] = '\0';
  gpm_report (GPM_PR_DEBUG,"serial tossing: %s", junk);
#endif
}

/* write 'mode' to a serial touchpad, STIG 58 */
static void syn_serial_set_mode (int fd,
				 unsigned char mode) 
{
  unsigned char bytes [15];

  sprintf (bytes, "%%C3B%02X5555", mode);
#if DEBUG_SENT_DATA
  gpm_report (GPM_PR_DEBUG,"modes: %s", bytes);
#endif
  tp_serial_send_cmd (fd, bytes);
}

/* read the identification from the serial touchpad, STIG 57*/
static void syn_serial_read_ident (int fd,
				   info_type *info) 
{
  unsigned char bytes [5];

  tp_serial_send_cmd (fd, "%A");
  tp_serial_read (fd, bytes, 4);

#if DEBUG_SENT_DATA
  bytes [4] = '\0';
  gpm_report (GPM_PR_INFO,"Serial ident: %s", bytes);
#endif

  if ( bytes [0] == 'S' && bytes [0] == 'T'){
    /* reformat the data
     * The out commented is wrong according to STIG page 57.*/
    /* info->info_model_code = (bytes [2] & 0x07) >> 3; */
    info->info_model_code = bytes [2] >> 3;
    info->info_major      = (bytes [2] & 0x07);
    info->info_minor      = bytes [3];
  }else{
    gpm_report (GPM_PR_ERR,"PS/2 serial device doesn't appear to be a synaptics touchpad\n");
    info->info_model_code = 0;
    info->info_minor      = 0;
    info->info_major      = 0;
  }
}



/* read the model_id from the serial touchpad (in ps/2 format) */
static void syn_serial_read_model_id (int fd,
				      model_id_type *model) 
{
  unsigned char bytes [7];
  int model_int;

  /* 
   * for older touchpads this command is not supported and no response will
   * come.  We should do non blocking input here to handle that case 
   * and return byte2 as 0x47 ... later.
   *
   * pebl: It is easier just to check version number less than 3.2, STIG page 60.
   */
  if ( (ident[0].info_major >= 4) ||
       (ident[0].info_major == 3 && ident[0].info_minor >= 2)){
    tp_serial_send_cmd (fd, "%D");
    tp_serial_read (fd, bytes, 6);

    /* reformat the data */
    model_int = ((tp_hextoint (bytes [0], bytes [1]) << 16) |
		 (tp_hextoint (bytes [2], bytes [3]) << 8) |
		 (tp_hextoint (bytes [4], bytes [5])));
    
#   if DEBUG_SENT_DATA
      bytes [6] = '\0';
      gpm_report (GPM_PR_DEBUG,"Serial model id: %s", bytes);
#   endif

  }else{
    model_int = 0;
  }

  syn_extract_model_id_info (model_int, model);
}


/* read the mode bytes and capabilities from the serial touchpad, STIG 57 */
static void syn_serial_read_cap (int fd,
				 ext_cap_type *cap) 
{
  unsigned char bytes [8];
  int cap_int = 0;

  tp_serial_send_cmd (fd, "%B");
  tp_serial_read (fd, bytes, 8);

#if DEBUG_SENT_DATA
  bytes [7] = '\0';
  gpm_report (GPM_PR_DEBUG,"Serial capabilites: %s", bytes);
#endif

  if (ident[0].info_major >= 4){
    if (bytes [0] == '3' && bytes [0] == 'B'){
      cap_int = ((tp_hextoint (bytes [4], bytes [5]) << 8) |
		 (tp_hextoint (bytes [6], bytes [7])));
    }else{
      gpm_report (GPM_PR_ERR,"PS/2 serial device doesn't appear to be a synaptics touchpad\n");
    }    
  }

  syn_extract_extended_capabilities(cap_int,cap);
}



/*------------------------------------------------------------------------*/
/*   PS/2 Utility functions.                                              */
/*      Adapted from tpconfig.c by C. Scott Ananian                       */
/*------------------------------------------------------------------------*/

/* PS2 Synaptics is using LSB, STIG page 29. 
**
** After power on or reset the touchpads are set to these defaults:
** 100 samples per second
** Resolution is 4 counts per mm
** Scaling 1:1
** Stream mode is selected
** Data reporting is disabled
** Absolute mode is disabled
*/


/* Normal ps2 commands, Command set is on STIG page 33  */
#define PS2_RESET         0xFF      /* Reset */
#define PS2_RESEND        0xFE      /* Resend command */
#define PS2_SET_DEFAULT   0xF6      /* Set default */
#define PS2_DISABLE_DATA  0xF5      /* Stop sending motion data packets */
#define PS2_ENABLE_DATA   0xF4      /* Start sending motion data packets */
#define PS2_SAMPLE_RATE   0xF3      /* Set sample rate to value in a following byte */
#define PS2_READ_DEVICE   0xF2      /* Read device type (ack and idcode) */
#define PS2_REMOTE_MODE   0xF0      /* Set touchpad in remote mode */
#define PS2_WRAP_MODE     0xEE      /* Enter wrap mode */
#define PS2_END_WRAP_MODE 0xEE      /* Leave wrap mode */
#define PS2_READ_DATA     0xEB      /* Read move data (in remote mode)*/
#define PS2_STREAM_MODE   0xEA      /* Enter stream mode (device sends move data) */
#define PS2_STATUS_REQ    0xE9      /* Ask for status request */
#define PS2_RESOLUTION    0xE8      /* Set resolution to next transmitted byte */
#define PS2_SCALE_12      0xE7      /* Set scale to 1:2 */
#define PS2_SCALE_11      0xE6      /* Set scale to 1:1 */


/* Normal ps2 responce */
#define PS2_ERROR         0xFC      /* Error, after a reset,resend or disconnect*/
#define PS2_ACK           0xFA      /* Command acknowledge */
#define PS2_READY         0xAA      /* Send after a calibration or ERROR */
#define PS2_MOUSE_IDCODE  0x00      /* Identification code (meaning mouse) sent after a PS2_READY */


/* Additional synaptic commands*/
#define PS2_SYN_CMD       0xE8      /* Four of these each with an following byte encodes a command*/
#define PS2_SYN_INERT     0xE6      /* This ps2 command is ignored by synaptics */
#define PS2_SYN_SET_MODE1 0x0A      /* Set the mode byte 1 instead of sample rate (used after a sample rate cmd) */
#define PS2_SYN_SET_MODE2 0x14      /* Set the mode byte 2 instead of sample rate (used after a sample rate cmd).
				     * All other sample rate gives undefined behavior (used to address 4 byte mode)*/
#define PS2_SYN_SET_STICK 0x28      /* Send byte to stick */
#define PS2_SYN_STATUS_OK 0x47      /* Special synaptics Status report is recognized */


/* These are the commands that can be given (encoded) by PS_SYN_CMD */
#define PS2_SYN_CMD_IDENTIFY     0x00 /* Identify Touchpad */
#define PS2_SYN_CMD_MODES        0x01 /* Read Touchpad Modes */
#define PS2_SYN_CMD_CAPABILITIES 0x02 /* Read capabilities */
#define PS2_SYN_CMD_MODEL_ID     0x03 /* Read model id */
#define PS2_SYN_CMD_SERIAL_NO_P  0x06 /* Read serial number prefix */
#define PS2_SYN_CMD_SERIAL_NO_S  0x07 /* Read serial number suffix */
#define PS2_SYN_CMD_RESOLUTIONS  0x08 /* Read resolutions */




/* read a byte from the ps/2 port */
static byte tp_ps2_getbyte(int fd) 
{
  byte b;

# ifdef DEBUG_GETBYTE
  gpm_report(GPM_PR_DEBUG,"Getting byte");
# endif

  read(fd, &b, 1);

# ifdef DEBUG_GETBYTE
  gpm_report(GPM_PR_DEBUG,"Got %X",b);
# endif

  return b;
}


/* write a byte to the ps/2 port, handling resend.*/
static byte tp_ps2_putbyte(int fd,
			   byte b) 
{
  byte ack;

# ifdef DEBUG_PUTBYTE
  gpm_report(GPM_PR_DEBUG,"Send real byte %X",b);
# endif

  write(fd, &b, 1);
  read(fd, &ack, 1);

  if (ack == PS2_RESEND) {
    write(fd, &b, 1);
    read(fd, &ack, 1);
  }

# ifdef DEBUG_PUTBYTE_ACK
  gpm_report(GPM_PR_DEBUG,"Responce %X to byte %X",ack,b);
# endif

  return ack;
}


/* Read a byte from the touchpad or use the Synaptics extended ps/2 syntax to
 * read a byte from the stick device. The variable stick is used to indicate
 * whether it is the touchpad or stick device that is meant.
 */
static byte syn_ps2_getbyte(int fd, int stick) 
{
  byte response[6];

  if (!stick) {
    response[1]=tp_ps2_getbyte(fd);
  } else {
    response[0]=tp_ps2_getbyte(fd);
    response[1]=tp_ps2_getbyte(fd);
    response[2]=tp_ps2_getbyte(fd);
    response[3]=tp_ps2_getbyte(fd);
    response[4]=tp_ps2_getbyte(fd);
    response[5]=tp_ps2_getbyte(fd);
    
    /* Do some sanity checking */
    if((response[0] & 0xFC) != 0x84) {
      gpm_report (GPM_PR_ERR,"Byte 0 of stick device responce is not valid");
      return -1;
    }
    if((response[3] & 0xCC) != 0xC4) {
      gpm_report (GPM_PR_ERR,"Byte 3 of stick device responce is not valid");
      return -1;
    }
  }

  return response[1];
}


/* write byte to the touchpad or use the Synaptics extended ps/2 syntax to write
 * a byte to the stick device. The variable stick is used to indicate
 * whether it is the touchpad or stick device that is meant.  */
static void syn_ps2_putbyte(int fd,
			    int stick,
			    byte b) 
{
  byte ack;

# ifdef DEBUG_PUTBYTE
  gpm_report(GPM_PR_DEBUG,"Send byte %X to %s",b,(stick?"Stick":"Touchpad"));
# endif

  if (!stick) {
    ack = tp_ps2_putbyte(fd,b);
  } else {
    syn_ps2_send_cmd(fd, DEVICE_TOUCHPAD, b);
    ack = tp_ps2_putbyte(fd, PS2_SAMPLE_RATE);
    if (ack != PS2_ACK)
      gpm_report (GPM_PR_ERR,"Invalid ACK to stick putbytet sample rate");
    ack = tp_ps2_putbyte(fd, PS2_SYN_SET_STICK);
    if (ack != PS2_ACK)
      gpm_report (GPM_PR_ERR,"Invalid ACK to stick putbytet set stick");
    ack = syn_ps2_getbyte(fd,DEVICE_STICK);
  }

  if (ack != PS2_ACK)
    gpm_report (GPM_PR_ERR,"Invalid ACK to synps2 %s putbyte %02X, got %02X",
		(stick?"Stick":"Touchpad"),b,ack);
}


/* use the Synaptics extended ps/2 syntax to write a special command byte 
* STIG page 36: Send exactly four PS2_SYN_CMD (which is otherwise ignored)
* and after each a byte with the 2 first bits being the command (LSB). End it with
* either  PS2_SAMPLE_RATE or PS2_STATUS_REQ. It is hinted to send an inert command
* first so not having five or more PS2_SYN_CMD by coincident.
*
* If data is for the stick device every byte has to be encode by the above method.
*/
static void syn_ps2_send_cmd(int fd,
			     int stick, 
			     byte cmd)
{
  int i;

# ifdef DEBUG_CMD
  gpm_report(GPM_PR_DEBUG,"Send Command %X to %s",cmd,(stick?"Stick":"Touchpad"));
# endif

  /* initialize with 'inert' command */
  tp_ps2_putbyte(fd, PS2_SYN_INERT);
  for (i=0; i<4; i++) {
    syn_ps2_putbyte(fd, stick, PS2_SYN_CMD);
    syn_ps2_putbyte(fd, stick, (cmd>>6)&0x3);
    cmd<<=2;
  }
}

#if 0

/* write 'cmd' to mode byte 1.
 * This function is not used. 
 * Code 0x0A is unknown to me, maybe used in older synaptics?
 *
 * This is used for some old 2 byte control synaptics. The function is not
 * used, and is probably leftover from mixing with Van der Plas code.
 */
static void syn_ps2_set_mode1(int fd,
			      int stick, 
			      byte cmd)
{
  syn_ps2_send_cmd(fd, stick, cmd);
  tp_ps2_putbyte(fd, stick, PS2_SAMPLE_RATE);
  tp_ps2_putbyte(fd, stick, PS2_SYN_SET_MODE1);
}

#endif


/* write 'cmd' to mode byte 2 
 * See ps2_send_cmd. PS2_SR_SET_MODE stores the touchpad mode encoded in the 
 * four PS2_SYN_CMD commands
 */
static void syn_ps2_set_mode2(int fd,
			      int stick,
			      byte cmd)
{
  syn_ps2_send_cmd(fd, stick, cmd);
  syn_ps2_putbyte (fd, stick, PS2_SAMPLE_RATE);
  syn_ps2_putbyte (fd, stick, PS2_SYN_SET_MODE2);
}


/* read three byte status ('a','b','c') corresponding to register 'cmd' 
*  Special status request for synaptics is given after a cmd.
*  Byte b is PS2_SYN_STATUS_OK to recognize a synaptics
*/
static void syn_ps2_status_rqst(int fd,
				int stick,
				byte cmd,
				byte *bytes)
{
  gpm_report (GPM_PR_INFO,"Status request for %s, %X", (stick?"stick":"touchpad"),cmd);

  syn_ps2_send_cmd(fd, stick, cmd);
  syn_ps2_putbyte (fd, stick, PS2_STATUS_REQ);
  bytes [0]=syn_ps2_getbyte(fd,stick);
  bytes [1]=syn_ps2_getbyte(fd,stick);
  bytes [2]=syn_ps2_getbyte(fd,stick);

  gpm_report (GPM_PR_INFO,"Status request %X %X %X", bytes[0], bytes[1], bytes[2]);
}


#if 0

/* read the modes from the touchpad (in ps/2 format) */
static void syn_ps2_read_modes (int fd, int stick) 
{
  unsigned char bytes [3];

  syn_ps2_status_rqst (fd, stick, PS2_SYN_CMD_MODES, bytes);
# ifdef DEBUG
  gpm_report (GPM_PR_INFO,"Synaptic PS/2 %s modes: %02X", (stick?"stick":"touchpad"),bytes [2]);
# endif
}

#endif


/* read the identification from the ps2 touchpad */
static void syn_ps2_read_ident (int fd,
				int stick, 
				info_type *info)
{
  byte bytes [3];

  syn_ps2_status_rqst (fd, stick, PS2_SYN_CMD_IDENTIFY, bytes);
  if (bytes [1] != PS2_SYN_STATUS_OK) {
    gpm_report (GPM_PR_ERR,"PS/2 device doesn't appear to have synaptics %s identification\n",
		(stick?"sticks":"touchpads"));
    info->info_minor      = 0;
    info->info_model_code = 0;
    info->info_major      = 0;

  } else {
    info->info_minor      = bytes [0];
    info->info_model_code = (bytes [2] >> 4) & 0x0f;
    info->info_major      = bytes [2] & 0x0f;
  }
}


/* read the model_id from the ps2 touchpad/stick */
static void syn_ps2_read_model_id (int fd,
				   int stick, 
				   model_id_type *model)
{
  unsigned char bytes [3];
  int model_int;

  syn_ps2_status_rqst (fd, stick, PS2_SYN_CMD_MODEL_ID, bytes);
  model_int = ((bytes [0] << 16) |
	       (bytes [1] << 8)  |
	       (bytes [2]));
  syn_extract_model_id_info (model_int, model);
}


/* read the extended capability from the ps2 touchpad, STIG page 15 */
static void syn_ps2_read_cap (int fd,
			      int stick,
                              ext_cap_type *cap)
{
  unsigned char bytes [3];
  int ext_cap_int;

  syn_ps2_status_rqst (fd, stick, PS2_SYN_CMD_CAPABILITIES, bytes);

  if (bytes [1] != PS2_SYN_STATUS_OK) {
    gpm_report (GPM_PR_ERR,"PS/2 device doesn't appear to have synaptics %s capabilities\n",
		(stick?"stick":"touchpad"));
    ext_cap_int = 0;
  }else{
    ext_cap_int = bytes[0] << 8 | bytes[2];
  }

  syn_extract_extended_capabilities(ext_cap_int, cap);
}



/*
 * ps2_disable_data
 *
 * Disable data reporting (streaming), and flush eventual old packets. As the
 * kernel keeps a queue of received data from the touchpad, the next byte we
 * read could be old data and not the ack to our command or request.  Disable
 * data should always be called before sending commands or request to the
 * touchpad.
 *
 * Note that this is a general ps2 command which should not be in this file,
 * but in mice.c.
 */
static void tp_ps2_disable_data (int fd) 
{
  struct timeval tv;
  fd_set rfds;
  unsigned char status;
  byte cmd = PS2_DISABLE_DATA;

  
  FD_ZERO(&rfds);
  FD_SET (fd, &rfds);
  tv.tv_sec = 0;
  tv.tv_usec = 0;

  write(fd,&cmd,1);

  usleep (50000);
  
  while (select (fd+1, &rfds, NULL, NULL, &tv) == 1) {
    read (fd, &status, 1);
#if DEBUG_RESET
    gpm_report (GPM_PR_INFO,"PS/2 device disable data flush: %02X", status);
#endif
    FD_ZERO(&rfds);
    FD_SET (fd, &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;
  }

  if (status != PS2_ACK)
    gpm_report (GPM_PR_ERR,"Invalid ACK to disable stream command, got %02X",status);
}


/* 
 * syn_ps2_enable_data
 *
 * Enable data after a disable data command. Should have called the disable data
 * before calling this function.
 */

static void syn_ps2_enable_data(int fd)
{  
  if (stick_enabled)
    syn_ps2_putbyte(fd,DEVICE_STICK,PS2_ENABLE_DATA);
  syn_ps2_putbyte(fd,DEVICE_TOUCHPAD,PS2_ENABLE_DATA);
}


/* 
 * syn_ps2_send_reset
 *
 * Send reset command and absorb additional READY, IDCODE from the
 * touchpad. Should have called the disable data before calling this function.
 * Synaptics garanties always to return PS2_READY. STIG page 31 and 48.
 */

static void syn_ps2_send_reset(int fd, int stick)
{
  byte status,id_code = PS2_MOUSE_IDCODE;
  byte reset_cmd = PS2_RESET;

  gpm_report(GPM_PR_DEBUG,"Reseting Synaptic PS/2 %s\n",(stick?"Stick":"Touchpad"));

  /* Send reset command without eating the ack. */
  if(!stick) {
    write(fd,&reset_cmd,1);
    status = tp_ps2_getbyte(fd);
  } else {
    syn_ps2_putbyte(fd, stick, reset_cmd);
    status = PS2_ACK;
  }

  /* Sometimes the touchpad sends additional ready,idcode before ack the reset command.
   * I dont know why! */
  while (status == PS2_READY){
    gpm_report(GPM_PR_INFO,"PS/2 device sending additional READY, ID CODE.\n");
    id_code = syn_ps2_getbyte(fd,stick);
    status  = syn_ps2_getbyte(fd,stick);
  }

  if (status != PS2_ACK || id_code != PS2_MOUSE_IDCODE){
    gpm_report(GPM_PR_ERR,"Sending reset command to PS/2 Device failed: No ACK, got %02X.\n",status);
  }

  /* Wait 750 ms to recalibrate. */
  usleep(750000);

  if ( (status  = syn_ps2_getbyte(fd,stick)) != PS2_READY ){
    gpm_report(GPM_PR_ERR,"Reseting PS/2 Device failed: No READY, got %02X.\n"
	                  "Check pc_keyb.c for reconnect smartness.\n",status);
  }
  if ( (id_code = syn_ps2_getbyte(fd,stick)) != PS2_MOUSE_IDCODE){
    gpm_report(GPM_PR_ERR,"Reseting PS/2 Device failed: Wrong ID, got %02X.\n",id_code);
  }

}


/*
** syn_ps2_absolute_mode
**
** Put the touchpad into absolute mode.
*/

static void syn_ps2_absolute_mode(int fd)
{

  /* select 6 byte packet, high packet rate, no-sleep */
  syn_ps2_set_mode2 (fd, DEVICE_TOUCHPAD, 
		     (ABSOLUTE_MODE    |
		      HIGH_REPORT_RATE |
		      PS2_NO_SLEEP     |
		      (wmode_enabled ? NO_TAPDRAG_GESTURE : TAPDRAG_GESTURE) |
		      (stick_enabled ? STICK_ENABLED : STICK_DISABLE)        |
		      (wmode_enabled ? REPORT_W_ON : REPORT_W_OFF)));

}



/****************************************************************************
**
**  TRANSLATE FUNCTIONS the incoming data to an uniform report
**
****************************************************************************/

/* SERIAL PACKETS
 * STIG page 63, note 7 bit!
 *
 * byte 0 |    1   | reserved | gesture  | finger |  left   |  middle | right|  
 * byte 1 |    0   |                     x-pos 7-12                          |
 * byte 2 |    0   |                     x-pos 1-6                           |
 * byte 3 |    0   |                     y-pos 7-12                          |
 * byte 4 |    0   |                     y-pos 1-6                           |
 * byte 5 |    0   |                  z pressure 7-2                         |
 * byte 6 |    0   |  down    |   up     |y-pos 0 | x-pos 0 | z pressure 0-1 |  (new_abs set)
 * byte 7 |    0   |      reserved       |       W mode 0-3                  |  (new_abs and wmode set)
 */


static void syn_serial_translate_data (unsigned char *data,
					 report_type *report) 
{
  report->gesture     = check_bits (data [0], 0x10);
  report->fingers     = check_bits (data [0], 0x08);
  report->left        = check_bits (data [0], 0x04);
  report->middle      = check_bits (data [0], 0x02);
  report->right       = check_bits (data [0], 0x01);
  report->x           = (data [1] << 7) | (data [2] << 1);
  report->y           = (data [3] << 7) | (data [4] << 1);
  report->pressure    = data [5] << 2;
  report->fourth      = 0;
  report->up          = 0;
  report->down        = 0;
  report->w           = 0;
  report->fingerwidth = 0;

  if (model[0].info_new_abs){
    report->up        = check_bits (data [6], 0x20);
    report->down      = check_bits (data [6], 0x40);
    report->y        |= (data [6] & 0x08) >> 3;
    report->x        |= (data [6] & 0x04) >> 2;
    report->pressure |= (data [6] & 0x03);

    if (wmode_enabled){
      report->w         = (data [7] & 0x0F);
    }
  }
}


/* PS2 PACKETS 
 *
 * Handle error packets. It may be garbage or not, as the synaptics pad keeps sending data 1
 * sec after last touch.
 */

static void syn_ps2_translate_error(unsigned char *data,
				    report_type *report)
{
  gpm_report(GPM_PR_WARN,"Unrecognized Synaptic PS/2 Touchpad packet: %02X %02X %02X %02X %02X %02X",
	     data [0],data [1],data [2],data [3],data [4],data [5]);

  if (reset_on_error_enabled) {
    /* Hack to get the fd: which_mouse is the current mouse,
       and as the synaptic code is called, it is the current mouse. */
    syn_ps2_reset(which_mouse->fd);
    syn_ps2_absolute_mode(which_mouse->fd);
  }
  
  report->left        = 0;
  report->middle      = 0;
  report->right       = 0;
  report->fourth      = 0;
  report->up          = 0;
  report->down        = 0;
  report->x           = 0;
  report->y           = 0;
  report->pressure    = 0;
  report->gesture     = 0;
  report->fingers     = 0;
  report->fingerwidth = 0;
  report->w           = 0;
  
}


/*
 * STIG page 42
 * wmode = 0, newer version. Gesture, right and left are repeated.
 *
 * byte 0 |     1   |    0   |  Finger  | Reserved |    0   | Gesture |  Right  | Left |  
 * byte 1 |         y-pos 11-8                     |         x-pos  11-8               |
 * byte 2 |                           z pressure 0-7                                   |
 * byte 3 |     1   |    1   | y-pos 12 | x-pos 12 |    0   | Gesture |  Right  | Left |
 * byte 4 |                                 x - pos 0-7                                |
 * byte 5 |                                 y - pos 0-7                                |
 *
 * STIG page 43
 * wmode = 0, old version <  3.2.
 * Second is a second gesture!?
 *
 * byte 0 |     1   |    1   |  z-pres 6-7         | Second | Gesture |  Right  | Left |  
 * byte 1 |  finger |    0   |     0    |                    x-pos  12-8               |
 * byte 2 |                           x-pos  0-7                                       |
 * byte 3 |     1   |    0   |               z-pressure 0-5                            |
 * byte 4 |Reserved |    0   |    0     |              y - pos 8-12                    |
 * byte 5 |                                 y - pos 0-7                                |
 * 
 */

/* Translate the reported data into a record for processing */
static void syn_ps2_translate_data (unsigned char *data,
				    report_type *report) 
{
  
  /* Check that this is indeed an absolute 6 byte new version packet*/
  if (((data [0] & 0xc8) == 0x80) &&              /* Check static in byte 0 */
      ((data [3] & 0xc8) == 0xc0) &&              /* Check static in byte 3 */
      ((data [0] & 0x0F) == (data [3] & 0x0F))) { /* check repeated data */
    report->left        = check_bits (data [0], 0x01);
    report->middle      = 0;
    report->right       = check_bits (data [0], 0x02);
    report->fourth      = 0;
    report->up          = 0;
    report->down        = 0;
    report->x           = (((data [1] & 0x0F) << 8) |
			   ((data [3] & 0x10) << 8) |
			   ((data [4])));
    report->y           = (((data [1] & 0xF0) << 4) |
			   ((data [3] & 0x20) << 7) |
			   ((data [5])));
    report->pressure    = data [2];
    report->gesture     = check_bits (data [0], 0x04);
    report->fingers     = check_bits (data [0], 0x20);
    report->fingerwidth = 0;
    report->w           = 0;
    
  } /*th Old style packet maybe */
  else if (((data [0] & 0xC0) == 0xC0) && /* Static in byte 0*/
	   ((data [1] & 0x60) == 0x00) && /* Static in byte 1*/
	   ((data [3] & 0xC0) == 0x80) && /* Static in byte 3*/
	   ((data [4] & 0x60) == 0x00)) { /* Static in byte 4*/
    report->left        = check_bits (data [0], 0x01);
    report->middle      = 0;
    report->right       = check_bits (data [0], 0x02);
    report->fourth      = 0;
    report->up          = 0;
    report->down        = 0;
    report->x           = (((data [1] & 0x1F) << 8) |
			   ((data [2])));
    report->y           = (((data [4] & 0x1f) << 8) |
			   ((data [5])));
    report->pressure    = (((data [0] & 0x30) << 2 ) |
			   ((data [3] & 0x3f)));
    report->gesture     = check_bits (data [0], 0x04);
    report->fingers     = check_bits (data [1], 0x80);
    report->fingerwidth = 0;
    report->w           = 0;
    
  } else { 
    syn_ps2_translate_error(data,report);
  }
}


/* STIG page 42
 * wmode = 1, 
 *
 * byte 0 |     1   |    0   |  W 2-3              |    0   | W 1     |  Right  | Left |  
 * byte 1 |         y-pos 11-8                     |         x-pos  11-8               |
 * byte 2 |                           z pressure 0-7                                   |
 * byte 3 |     1   |    1   | y-pos 12 | x-pos 12 |    0   | W 0     |  R/D    | L/U  |
 * byte 4 |                                 x - pos 0-7                                |
 * byte 5 |                                 y - pos 0-7                                |
 *
 */

static void syn_ps2_translate_wmode_data (unsigned char *data,
					    report_type *report) 
{
  /* Check that it is an absolute packet */
  if (((data[0] & 0xc8) == 0x80) && ((data[3] & 0xc8) == 0xc0)) {

    report->left        = check_bits (data[0], 0x01);
    report->middle      = check_bits (data[0] ^ data[3], 0x01);
    report->right       = check_bits (data[0], 0x02);
    report->fourth      = check_bits (data[0] ^ data[3], 0x02);
    report->up          = 0;
    report->down        = 0;
    report->x           = (((data[1] & 0x0F) << 8) |
			   ((data[3] & 0x10) << 8) |
			   ((data[4])));
    report->y           = (((data[1] & 0xF0) << 4) |
			   ((data[3] & 0x20) << 7) |
			   ((data[5])));
    report->pressure    = data[2];
    report->fingers     = 0;
    report->fingerwidth = 0;
    report->gesture     = 0;
    report->w = (((data[3] & 0x04) >> 2) |
		 ((data[0] & 0x04) >> 1) |
		 ((data[0] & 0x30) >> 2));


  } else { 
    syn_ps2_translate_error(data,report);
  }
}


/****************************************************************************
**
** INTERFACE ROUTINES
**
****************************************************************************/

/*
** syn_process_serial_data
**
** Process the touchpad 6 byte report.
*/
void syn_process_serial_data (Gpm_Event *state,
			      unsigned char *data) 
{
  /* initialize the state */
  state->buttons = 0;
  state->dx      = 0;
  state->dy      = 0;

  syn_serial_translate_data (data, &cur_report);
  if (wmode_enabled){
    syn_process_wmode_report(&cur_report);
  }    
  if (tp_find_fingers(&cur_report,state)) return;
  if (wmode_enabled){
    tp_find_gestures(&cur_report);    
  }    

  tp_process_report (state, &cur_report);
}


/*
** syn_serial_reset
**
** Reset the touchpad to relative mode.  This cannot be called directly as a
** command should be sent in 1200 baud, not 9600.
*/

void syn_serial_reset(int fd)
{
  gpm_report (GPM_PR_INFO,"Reseting Synaptic Serial Touchpad.");

  syn_serial_set_mode (fd, (RELATIVE_MODE    |
			    HIGH_REPORT_RATE |
			    USE_9600_BAUD    |
			    NORMAL_REPORT    |
			    REPORT_W_OFF));

}

/*
** syn_serial_init
** 
** Initialize the synaptics touchpad.  Read model and identification.
** Determine the size of the touchpad in "pixels".  Select 6/7/8 byte packets,
** select 9600 baud, and select high packet rate.
*/
int syn_serial_init (int fd) 
{
  int return_packetlength;

  gpm_report(GPM_PR_DEBUG,"Initializing Synaptics Serial TouchPad");

  syn_serial_read_ident (fd, &ident[0]);
  syn_serial_read_model_id (fd, &model[0]);
  syn_serial_read_cap(fd, &capabilities[0]);

  syn_process_config (ident[0], model[0]);

  syn_dump_info(0);

  /* Change the protocol to use either 6,7 or 8 bytes, STIG 63 */
  if (model[0].info_new_abs){
    if (wmode_enabled){
      return_packetlength = 8;
    }else{
      return_packetlength = 7;
    }
  }else{
    return_packetlength = 6;
    wmode_enabled = 0;
  }

  syn_serial_set_mode (fd, (ABSOLUTE_MODE    |
			    HIGH_REPORT_RATE |
			    USE_9600_BAUD    |
			    (model[0].info_new_abs ? EXTENDED_REPORT : NORMAL_REPORT) |
			    (wmode_enabled ? REPORT_W_ON : REPORT_W_OFF)));

  return return_packetlength;
}


/*
** syn_process_ps2_data
**
** Process the touchpad 6 byte report.
*/
void syn_process_ps2_data (Gpm_Event *state,
			   unsigned char *data) 
{
  /*   gpm_report(GPM_PR_DEBUG,"Data %02x %02x %02x %02x %02x %02x",data[0],data[1],data[2],data[3],data[4],data[5]); */

  /* initialize the state */
  state->buttons = 0;
  state->dx      = 0;
  state->dy      = 0;


  if (wmode_enabled) {
    syn_ps2_translate_wmode_data (data, &cur_report);
    if (syn_ps2_process_extended_packets(data,&cur_report,state)) return;      
    syn_process_wmode_report(&cur_report);
    if (tp_find_fingers(&cur_report,state)) return;
    tp_find_gestures(&cur_report);
  }else {
    syn_ps2_translate_data (data, &cur_report);
    if (tp_find_fingers(&cur_report,state)) return;
  }

  tp_process_report (state, &cur_report);
}


/*
** syn_ps2_reset
**
** Reset the touchpad and set to relative mode (ps/2).
*/

void syn_ps2_reset (int fd)
{
  gpm_report (GPM_PR_INFO,"Reseting Synaptic PS/2 Touchpad.");

  /* Stop incoming motion data (of whatever kind absolute/relative). */
  tp_ps2_disable_data(fd);

  if(stick_enabled)
    syn_ps2_send_reset(fd,DEVICE_STICK);
  syn_ps2_send_reset(fd,DEVICE_TOUCHPAD);

  syn_ps2_enable_data(fd);
}


/*
 * syn_ps2_init_stick
 *
 * Initialize the attached device on the synaptics touchpad (usually a stick).
 */

static void syn_ps2_init_stick(int fd)
{
  
  if (!stick_enabled) return;

  gpm_report(GPM_PR_DEBUG,"Initializing Synaptics PS/2 Stick Device");

  /* Reset it, set defaults, streaming */
  syn_ps2_send_reset(fd,DEVICE_STICK);
  syn_ps2_putbyte(fd,DEVICE_STICK,PS2_SET_DEFAULT);
  syn_ps2_putbyte(fd,DEVICE_STICK,PS2_STREAM_MODE);

  /* Unused */
  /*   syn_ps2_read_ident    (fd, DEVICE_STICK, &ident[1]); */
  /*   syn_ps2_read_model_id (fd, DEVICE_STICK, &model[1]); */
  /*   syn_ps2_read_cap      (fd, DEVICE_STICK, &capabilities[1]); */

  /*   syn_dump_info(DEVICE_STICK); */
}



/*
** syn_ps2_init
** 
** Initialize the synaptics touchpad.  Read model and identification.
** Determine the size of the touchpad in "pixels".  Select 6 byte packets,
** and select high packet rate.
*/
void syn_ps2_init (int fd) 
{

  gpm_report(GPM_PR_DEBUG,"Initializing Synaptics PS/2 TouchPad");

  tp_ps2_disable_data(fd);

  /* Init touchpad */
  syn_ps2_send_reset(fd,DEVICE_TOUCHPAD);

  syn_ps2_read_ident    (fd, DEVICE_TOUCHPAD, &ident[0]);
  syn_ps2_read_model_id (fd, DEVICE_TOUCHPAD, &model[0]);
  syn_ps2_read_cap      (fd, DEVICE_TOUCHPAD, &capabilities[0]);

  syn_process_config (ident[0], model[0]);
  syn_dump_info(DEVICE_TOUCHPAD);

  syn_ps2_absolute_mode(fd);

  /* Absolut mode must be set before Init Stick device*/
  syn_ps2_init_stick(fd);

  /* Enable absolut mode and streaming */
  syn_ps2_enable_data(fd);
}
