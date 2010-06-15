/*
 * synaptics.h - support for the synaptics serial and ps2 touchpads
 *
 * Copyright 1999   hdavies@ameritech.net (Henry Davies
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
*/


#ifndef _SYNAPTICS_H_
#define _SYNAPTICS_H_


/*
** syn_process_serial_data
**
** Process the touchpad 6/7/8 byte data.
*/
void syn_process_serial_data (Gpm_Event *state,
			      unsigned char *data);



/*
** syn_process_ps2_data
**
** Process the touchpad 6 byte data.
*/
void syn_process_ps2_data (Gpm_Event *state,
			   unsigned char *data);



/*
** syn_serial_init
** 
** Initialize the synaptics touchpad.  Read model and identification.
** Determine the size of the touchpad in "pixels".  Select 6/7/8 byte packets,
** select 9600 baud, and select high packet rate.
** Return how many bytes in a packet.
*/
int syn_serial_init (int fd);



/*
** syn_ps2_init
** 
** Initialize the synaptics touchpad.  Read model and identification.
** Determine the size of the touchpad in "pixels".  Select 6 byte packets,
** and select high packet rate.
*/
void syn_ps2_init (int fd);


/*
** syn_ps2_reset
** 
** Reset the synaptics touchpad. Touchpad ends in relative mode.
*/
void syn_ps2_reset (int fd);


#endif
