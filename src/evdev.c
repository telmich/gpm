/*
 * evdev.c - support for event input devices in linux 2.4 & 2.6
 *
 * Copyright (C) 2003        Dmitry Torokhov <dtor@mail.ru>
 * Based on XFree86 driver by Stefan Gmeiner & Peter Osterlund
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
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/select.h>
#include <sys/time.h>

#include <linux/input.h>

#include "headers/gpm.h"
#include "headers/gpmInt.h"
#include "headers/console.h"
#include "headers/message.h"
#include "headers/optparser.h"

#ifndef MSC_GESTURE
#define MSC_GESTURE     2
#endif

#ifndef EV_SYNC
#define EV_SYNC         0
#endif

#ifndef SYN_REPORT
#define SYN_REPORT      0
#endif

#ifndef HAVE_INPUT_ABSINFO
struct input_absinfo {
   int value;
   int minimum;
   int maximum;
   int fuzz;
   int flat;
};
#endif

enum evdev_type {
   EVDEV_RELATIVE,
   EVDEV_ABSOLUTE,
   EVDEV_TOUCHPAD,
   EVDEV_SYNAPTICS
};

enum touch_type {
   TOUCH_NONE,
   TOUCH_FINGERS,
   TOUCH_PALM
};

enum gesture_type {
   GESTURE_NONE,
   GESTURE_TAP_PENDING,
   GESTURE_TAP,
   GESTURE_DRAG_PENDING,
   GESTURE_DRAG,
   GESTURE_DOUBLE_TAP
};

enum edge_type {
   BOTTOM_EDGE = 1,
   TOP_EDGE = 2,
   LEFT_EDGE = 4,
   RIGHT_EDGE = 8,
   LEFT_BOTTOM_EDGE = BOTTOM_EDGE | LEFT_EDGE,
   RIGHT_BOTTOM_EDGE = BOTTOM_EDGE | RIGHT_EDGE,
   RIGHT_TOP_EDGE = TOP_EDGE | RIGHT_EDGE,
   LEFT_TOP_EDGE = TOP_EDGE | LEFT_EDGE
};

struct event_data {
   int dx, dy;
   int wdx, wdy;
   int abs_x, abs_y;
   int buttons;
   int pressure;
   int w;
   int synced;
};

struct touch_data {
   enum touch_type type;
   enum gesture_type gesture;
   int finger_count;
   int x, y;
   int buttons;
   int clicks;
   struct timeval start;
};

struct event_device {
   enum evdev_type type;
   int dont_sync;

   struct event_data pkt;
   int pkt_count;

   int prev_x[4], prev_y[4];
   int prev_pressure, avg_w;
   struct touch_data touch;

   int left_edge, right_edge;
   int top_edge, bottom_edge;
   int touch_high, touch_low;
   int tap_time, tap_move;
   int y_inverted;
};

struct evdev_capabilities {
   unsigned char evbits[EV_MAX/8 + 1];
   unsigned char keybits[KEY_MAX/8 + 1];
   unsigned char absbits[ABS_MAX/8 + 1];
   unsigned char mscbits[MSC_MAX/8 + 1];
};

#ifndef max
#define max(a,b) ((a)>(b) ? (a) : (b))
#endif

#define fx(i)  (evdev->prev_x[(evdev->pkt_count - (i)) & 03])
#define fy(i)  (evdev->prev_y[(evdev->pkt_count - (i)) & 03])

#define toggle_btn(btn, val) do { if (val) data->buttons |= (btn);\
                                  else     data->buttons &= ~(btn);\
                             } while (0)
#define GET_TIME(tv) (gettimeofday(&tv, (struct timezone *)NULL))
#define DIF_TIME(t1,t2) ((t2.tv_sec - t1.tv_sec) * 1000 + (t2.tv_usec - t1.tv_usec) / 1000)

#define test_bit(bit, array)    (((unsigned long *)array)[bit / (8*sizeof(unsigned long))] & (1 << (bit % (8*sizeof(unsigned long)))))

/* ------------- evdev protocol handling routines ---------------------*/

static void parse_input_event(struct input_event *event, struct event_data *data)
{
   switch (event->type) {
      case EV_REL:
         switch (event->code) {
            case REL_X:
               data->dx = (signed char)event->value;
               break;
            case REL_Y:
               data->dy = (signed char)event->value;
               break;
            case REL_WHEEL:
               data->wdy += event->value;
               break;
            case REL_HWHEEL:
               data->wdx += event->value;
               break;
         }
         break;

      case EV_ABS:
         switch (event->code) {
            case ABS_X:
               data->abs_x = event->value;
               break;

            case ABS_Y:
               data->abs_y = event->value;
               break;

            case ABS_PRESSURE:
               data->pressure = event->value;
               break;
         }
         break;

      case EV_MSC:
         switch (event->code) {
            case MSC_GESTURE:
               data->w = event->value;
               break;
         }
         break;


      case EV_KEY:
         switch(event->code) {
            case BTN_0:
            case BTN_TOUCH:
            case BTN_LEFT:
               toggle_btn(GPM_B_LEFT, event->value);
               break;
            case BTN_2:
            case BTN_STYLUS2:
            case BTN_SIDE:
            case BTN_MIDDLE:
               toggle_btn(GPM_B_MIDDLE, event->value);
               break;
            case BTN_STYLUS:
            case BTN_1:
            case BTN_RIGHT:
               toggle_btn(GPM_B_RIGHT, event->value);
               break;
         }
         break;

      case EV_SYNC:
         switch(event->code) {
            case SYN_REPORT:
               data->synced = 1;
               break;
         }
         break;
   }
}

static void tp_figure_deltas(struct event_device *evdev, struct Gpm_Event *state)
{
   struct event_data *pkt = &evdev->pkt;

   state->dx = state->dy = 0;
   if (evdev->touch.type == TOUCH_FINGERS) {
      fx(0) = pkt->abs_x;
      fy(0) = pkt->abs_y;
      if (evdev->pkt_count >= 2 &&
          evdev->touch.gesture != GESTURE_DRAG_PENDING) {
         state->dx = ((fx(0) - fx(1)) * 2 + (fx(1) - fx(2)) / 2) / 2; //SYN_REL_DECEL_FACTOR;
         state->dy = ((fy(0) - fy(1)) * 2 + (fy(1) - fy(2)) / 2) / 2; //SYN_REL_DECEL_FACTOR;
      }
      evdev->pkt_count++;
   } else  {
      evdev->pkt_count = 0;
   }
}

static enum touch_type tp_detect_touch(struct event_device *evdev)
{
   if (evdev->touch.type == TOUCH_FINGERS)
      return evdev->pkt.pressure > evdev->touch_low ? TOUCH_FINGERS : TOUCH_NONE;
   else
      return evdev->pkt.pressure > evdev->touch_high ? TOUCH_FINGERS : TOUCH_NONE;
}

static enum touch_type syn_detect_touch(struct event_device *evdev)
{
   struct event_data *pkt = &evdev->pkt;
   enum touch_type type = TOUCH_NONE;

   if (pkt->pressure > 200 || pkt->w > 10)
      return TOUCH_PALM; /* palm */

   if (pkt->abs_x == 0)
      evdev->avg_w = 0;
   else
      evdev->avg_w = (pkt->w - evdev->avg_w + 1) / 2;

   if (evdev->touch.type == TOUCH_FINGERS)
      type = pkt->pressure > evdev->touch_low ? TOUCH_FINGERS : TOUCH_NONE;
   else if (pkt->pressure > evdev->touch_high) {
      int safe_w = max(pkt->w, evdev->avg_w);

      if (pkt->w < 2)
         type = TOUCH_FINGERS;                  /* more than one finger -> not a palm */
      else if (safe_w < 6 && evdev->prev_pressure < evdev->touch_high)
         type = TOUCH_FINGERS;                  /* thin finger, distinct touch -> not a palm */
      else if (safe_w < 7 && evdev->prev_pressure < evdev->touch_high / 2)
         type = TOUCH_FINGERS;                  /* thin finger, distinct touch -> not a palm */
      else if (pkt->pressure > evdev->prev_pressure + 1)
         type = TOUCH_NONE;                     /* pressure not stable, may be a palm */
      else if (pkt->pressure < evdev->prev_pressure - 5)
         type = TOUCH_NONE;                     /* pressure not stable, may be a palm */
      else
         type = TOUCH_FINGERS;
   }

   evdev->prev_pressure = pkt->pressure;
   return type;
}

static enum edge_type tp_detect_edges(struct event_device *evdev, int x, int y)
{
   enum edge_type edge = 0;

   if (x > evdev->right_edge)
      edge |= RIGHT_EDGE;
   else if (x < evdev->left_edge)
      edge |= LEFT_EDGE;

   if (y > evdev->top_edge)
      edge |= TOP_EDGE;
   else if (y < evdev->bottom_edge)
      edge |= BOTTOM_EDGE;

   return edge;
}

static int tp_touch_expired(struct event_device *evdev)
{
   struct timeval now;

   GET_TIME(now);
   return DIF_TIME(evdev->touch.start, now) > evdev->tap_time;
}

static int tp_detect_tap(struct event_device *evdev)
{
   return evdev->touch.finger_count > 0 ||
          (!tp_touch_expired(evdev) &&
           abs(evdev->pkt.abs_x - evdev->touch.x) < evdev->tap_move &&
           abs(evdev->pkt.abs_y - evdev->touch.y) < evdev->tap_move);
}

static int tp_tap_to_buttons(struct event_device *evdev)
{
   enum edge_type edge;

   if (!evdev->touch.finger_count) {
      edge = tp_detect_edges(evdev, evdev->pkt.abs_x, evdev->pkt.abs_y);
      switch (edge) {
         case RIGHT_TOP_EDGE:
            return GPM_B_MIDDLE;
            break;
         case RIGHT_BOTTOM_EDGE:
            return GPM_B_RIGHT;
            break;
         default:
            return GPM_B_LEFT;
            break;
      }
   } else {
      switch (evdev->touch.finger_count) {
         case 2:
            return GPM_B_MIDDLE;
         case 3:
            return GPM_B_RIGHT;
         default:
            return GPM_B_LEFT;
      }
   }
}

static void tp_detect_gesture(struct event_device *evdev, int timed_out, struct Gpm_Event *state)
{
   struct touch_data *touch = &evdev->touch;
   int was_touching = touch->type == TOUCH_FINGERS;

   touch->type = timed_out ?
                     TOUCH_NONE : (evdev->type == EVDEV_SYNAPTICS ?
                                   syn_detect_touch(evdev) : tp_detect_touch(evdev));

   if (touch->type == TOUCH_FINGERS) {
      if (!was_touching) {
         GET_TIME(touch->start);
         if (touch->gesture == GESTURE_TAP_PENDING) {
            touch->gesture = GESTURE_DRAG_PENDING;
         } else {
            touch->x = evdev->pkt.abs_x;
            touch->y = evdev->pkt.abs_y;
            touch->buttons = 0;
         }
      } else if (touch->gesture == GESTURE_DRAG_PENDING && tp_touch_expired(evdev)) {
         touch->gesture = GESTURE_DRAG;
      }
   } else if (touch->type == TOUCH_NONE) {
      if (was_touching) {
         if (tp_detect_tap(evdev)) {
            if (touch->gesture == GESTURE_DRAG_PENDING) {
               touch->gesture = GESTURE_DOUBLE_TAP;
               touch->clicks = 4;
            } else {
               if ((touch->buttons = tp_tap_to_buttons(evdev)) == GPM_B_LEFT) {
                  touch->gesture = GESTURE_TAP_PENDING;
               } else {
                  touch->gesture = GESTURE_TAP;
                  touch->clicks = 2;
               }
            }
         } else {
            touch->gesture = GESTURE_NONE;
         }
      } else {
         if (touch->gesture == GESTURE_TAP_PENDING && tp_touch_expired(evdev)) {
            touch->gesture = GESTURE_TAP;
            touch->clicks = 2;
         }
      }
   }
}

static int compose_gpm_event(struct event_device *evdev, int timed_out, Gpm_Event *state)
{
   struct event_data *pkt = &evdev->pkt;
   int next_timeout = -1;

	state->buttons = pkt->buttons;
   state->wdx = pkt->wdx; state->wdy = pkt->wdy;

   switch (evdev->type) {
      case EVDEV_RELATIVE:
         if (!timed_out)
            state->dx = pkt->dx; state->dy = pkt->dy;
         break;

      case EVDEV_ABSOLUTE:
         if (!timed_out) {
            if (pkt->abs_x < evdev->left_edge)
               pkt->abs_x = evdev->left_edge;
            else if (pkt->abs_x > evdev->right_edge)
               pkt->abs_x = evdev->right_edge;

            if (pkt->abs_y < evdev->bottom_edge)
               pkt->abs_y = evdev->bottom_edge;
            else if (pkt->abs_y > evdev->top_edge)
               pkt->abs_y = evdev->top_edge;

            state->x = (pkt->abs_x - evdev->left_edge) *
                        console.max_x / (evdev->right_edge - evdev->left_edge);
            state->y = (pkt->abs_y - evdev->bottom_edge) *
                        console.max_y / (evdev->top_edge - evdev->bottom_edge);

            if (evdev->y_inverted) state->y = console.max_y - state->y;
         }
         break;

      case EVDEV_TOUCHPAD:
      case EVDEV_SYNAPTICS:
         tp_detect_gesture(evdev, timed_out, state);
         tp_figure_deltas(evdev, state);

         if (evdev->type == EVDEV_SYNAPTICS &&
             evdev->touch.type == TOUCH_FINGERS &&
             !tp_touch_expired(evdev)) {
            if (evdev->pkt.w == 0 && evdev->touch.finger_count == 0)
               evdev->touch.finger_count = 2;
            if (evdev->pkt.w == 1)
               evdev->touch.finger_count = 3;
         } else
            evdev->touch.finger_count = 0;

         switch(evdev->touch.gesture) {
            case GESTURE_DOUBLE_TAP:
            case GESTURE_TAP:
               if (evdev->touch.clicks <= 0) {
                  evdev->touch.gesture = GESTURE_NONE;
               } else {
                  if (--evdev->touch.clicks % 2)
                     state->buttons |= evdev->touch.buttons;
                  else
                     state->buttons &= ~evdev->touch.buttons;
                  next_timeout = 0;
               }
               break;
           case GESTURE_DRAG:
               state->buttons |= evdev->touch.buttons;
               break;
            case GESTURE_DRAG_PENDING:
               next_timeout = evdev->tap_time;
               break;
            case GESTURE_TAP_PENDING:
               next_timeout = evdev->tap_time;
               break;
            default:
               break;
         }

         break;
   }

   if (evdev->y_inverted) state->dy = -state->dy;

   return next_timeout;
}

int M_evdev(struct micedev *dev, struct miceopt *opts,
            unsigned char *data, struct Gpm_Event *state)
{
   struct event_device *evdev = dev->private;
   struct input_event *event = (struct input_event *)data;
   int timed_out = data == NULL;

   if (!timed_out)
      parse_input_event(event, &evdev->pkt);

   if (timed_out || evdev->pkt.synced || evdev->dont_sync) {
      dev->timeout = compose_gpm_event(evdev, timed_out, state);
      evdev->pkt.synced = 0;
      return 0;
   }

   dev->timeout = -1;
   return -1;
}

/* ------------- evdev initialization routines ---------------------*/

static void evdev_get_capabilities(int fd, struct evdev_capabilities *caps)
{
   memset(caps, 0, sizeof(*caps));
   if (ioctl(fd, EVIOCGBIT(0, EV_MAX), caps->evbits) < 0) {
      gpm_report(GPM_PR_OOPS, "evdev: cannot query device capabilities");
   }

   if (test_bit(EV_ABS, caps->evbits) &&
       ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(caps->absbits)), caps->absbits) < 0) {
      gpm_report(GPM_PR_OOPS, "evdev: cannot query ABS device capabilities");
   }

   if (test_bit(EV_KEY, caps->evbits) &&
       ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(caps->keybits)), caps->keybits) < 0) {
      gpm_report(GPM_PR_OOPS, "evdev: cannot query KEY device capabilities");
   }

   if (test_bit(EV_MSC, caps->evbits) &&
       ioctl(fd, EVIOCGBIT(EV_MSC, sizeof(caps->mscbits)), caps->mscbits) < 0) {
      /* dont complain as 2.4 kernels didnt have it
      gpm_report(GPM_PR_OOPS, "evdev: cannot query MSC device capabilities");
      */
   }
}

static void evdev_query_axis(int fd, int axis, int *axis_min, int *axis_max)
{
   struct input_absinfo axis_info;

   if (ioctl(fd, EVIOCGABS(axis), &axis_info) == 0) {
      *axis_min = axis_info.minimum;
      *axis_max = axis_info.maximum;
   } else
      gpm_report(GPM_PR_ERR, "evdev: could not query axis data");
}

static enum evdev_type evdev_guess_type(struct evdev_capabilities *caps)
{
   if (test_bit(EV_ABS, caps->evbits)) {
      if (caps->keybits[BTN_DIGI / 8] || !test_bit(ABS_PRESSURE, caps->absbits))
         return EVDEV_ABSOLUTE;

      return test_bit(EV_MSC, caps->evbits) && test_bit(MSC_GESTURE, caps->mscbits) ?
             EVDEV_SYNAPTICS : EVDEV_TOUCHPAD;
   }

   if (!test_bit(EV_REL, caps->evbits))
      gpm_report(GPM_PR_OOPS,
                 "evdev: device does not report neither absolute nor relative coordinates");

   return EVDEV_RELATIVE;
}

static enum evdev_type evdev_str_to_type(const char *type)
{
   if (!strcmp(type, "relative")) {
      return EVDEV_RELATIVE;
   } else if (!strcmp(type, "absolute")) {
      return EVDEV_ABSOLUTE;
   } else if (!strcmp(type, "touchpad")) {
      return EVDEV_TOUCHPAD;
   } else if (!strcmp(type, "synaptics")) {
      return EVDEV_SYNAPTICS;
   } else {
      gpm_report(GPM_PR_OOPS, "evdev: unknown type '%s'", type);
      return 0;  /* won't happen as gpm_report(OOPS) does not return */
   }
}

static void warn_if_present(struct option_helper *optinfo, const char *name, const char *type)
{
   if (is_option_present(optinfo, name))
      gpm_report(GPM_PR_WARN,
                 "evdev: option '%s' is not valud for type '%s', ignored",
                 name, type);
}

// -o type=(auto|synaptics|touchpad|relative|absolute),y_inverted,
//    left=1234,right=1234,top=1234,bottom=1234,
//    touch_high=30,touch_low=25,tap_time=30,tap_move=100
static void evdev_apply_options(struct event_device *evdev, char *optstring)
{
   char *type = "auto";
   struct option_helper optinfo[] = {
      { "type",         OPT_STRING, u: { sptr: &type } },
      { "y_inverted",   OPT_BOOL,   u: { iptr: &evdev->y_inverted }, value: 1 },
      { "left",         OPT_INT,    u: { iptr: &evdev->left_edge } },
      { "right",        OPT_INT,    u: { iptr: &evdev->right_edge } },
      { "top",          OPT_INT,    u: { iptr: &evdev->top_edge } },
      { "bottom",       OPT_INT,    u: { iptr: &evdev->bottom_edge } },
      { "touch_high",   OPT_INT,    u: { iptr: &evdev->touch_high } },
      { "touch_low",    OPT_INT,    u: { iptr: &evdev->touch_low } },
      { "tap_time",     OPT_INT,    u: { iptr: &evdev->tap_time } },
      { "tap_move",     OPT_INT,    u: { iptr: &evdev->tap_move } },
      { "",             OPT_END }
   };

   if (parse_options("evdev", optstring, ',', optinfo) < 0)
      gpm_report(GPM_PR_OOPS, "evdev: failed to parse option string");

   if (strcmp(type, "auto"))
      evdev->type = evdev_str_to_type(type);

   switch (evdev->type) {
      case EVDEV_RELATIVE:
         warn_if_present(optinfo, "left", type);
         warn_if_present(optinfo, "right", type);
         warn_if_present(optinfo, "top", type);
         warn_if_present(optinfo, "bottom", type);
         warn_if_present(optinfo, "tap_move", type);
         warn_if_present(optinfo, "tap_time", type);
         warn_if_present(optinfo, "touch_high", type);
         warn_if_present(optinfo, "touch_low", type);
         break;

      case EVDEV_ABSOLUTE:
         warn_if_present(optinfo, "tap_move", type);
         warn_if_present(optinfo, "tap_time", type);
         warn_if_present(optinfo, "touch_high", type);
         warn_if_present(optinfo, "touch_low", type);
         break;

      case EVDEV_TOUCHPAD:
         break;

      case EVDEV_SYNAPTICS:
         warn_if_present(optinfo, "y_inverted", type);
         break;
   }
}

int I_evdev(struct micedev *dev, struct miceopt *opts, Gpm_Type *type)
{
   struct evdev_capabilities caps;
   struct event_device *evdev;

   if (!dev->private) {          /* called first time, not re-init */
      if (!(dev->private = evdev = malloc(sizeof(*evdev))))
         gpm_report(GPM_PR_OOPS, "Can't allocate memory for event device");

      memset(evdev, 0, sizeof(*evdev));

      evdev_get_capabilities(dev->fd, &caps);
      evdev->type = evdev_guess_type(&caps);

      /* load default values - suitable for my synaptics ;P */
      evdev->left_edge = 1900;
      evdev->right_edge = 5400;
      evdev->top_edge = 3900;
      evdev->bottom_edge = 1800;
      evdev->tap_time = 180;
      evdev->tap_move = 220;
      evdev->touch_high = 30;
      evdev->touch_low = 25;

      if (evdev->type == EVDEV_ABSOLUTE) {
         if (test_bit(ABS_X, caps.absbits))
            evdev_query_axis(dev->fd, ABS_X, &evdev->left_edge, &evdev->right_edge);
         if (test_bit(ABS_Y, caps.absbits))
            evdev_query_axis(dev->fd, ABS_Y, &evdev->bottom_edge, &evdev->top_edge);
      }

      evdev_apply_options(evdev, opts->text);

      if (!test_bit(EV_SYNC, caps.evbits)) {
         evdev->dont_sync = 1;
         if (evdev->type == EVDEV_TOUCHPAD || evdev->type == EVDEV_SYNAPTICS)
            gpm_report(GPM_PR_OOPS,
                       "evdev: The running kernel lacks EV_SYNC support which is required for touchpad/synaptics mode");
      }

      switch (evdev->type) {
         case EVDEV_RELATIVE:
            gpm_report(GPM_PR_INFO, "evdev: selected Relative mode");
            if (!test_bit(EV_REL, caps.evbits))
               gpm_report(GPM_PR_WARN, "evdev: selected relative mode but device does not report any relative events");
            break;

         case EVDEV_ABSOLUTE:
            gpm_report(GPM_PR_INFO, "evdev: selected Absolute mode");
            if (evdev->right_edge <= evdev->left_edge)
               gpm_report(GPM_PR_OOPS, "evdev: right edge value should be gerater then left");
            if (evdev->top_edge <= evdev->bottom_edge)
               gpm_report(GPM_PR_OOPS, "evdev: top edge value should be gerater then bottom");
            if (!test_bit(EV_ABS, caps.evbits))
               gpm_report(GPM_PR_WARN, "evdev: selected absolute mode but device does not report any absolute events");
            opts->absolute = 1;
            break;

         case EVDEV_TOUCHPAD:
            gpm_report(GPM_PR_INFO, "evdev: selected Touchpad mode");
            if (!test_bit(EV_ABS, caps.evbits))
               gpm_report(GPM_PR_WARN, "evdev: selected touchpad mode but device does not report any absolute events");
            if (!test_bit(ABS_PRESSURE, caps.absbits))
               gpm_report(GPM_PR_WARN, "evdev: selected touchpad mode but device does not report pressure");
            break;

         case EVDEV_SYNAPTICS:
            gpm_report(GPM_PR_INFO, "evdev: selected Synaptics mode");
            if (!test_bit(EV_ABS, caps.evbits))
               gpm_report(GPM_PR_WARN, "evdev: selected synaptics mode but device does not report any absolute events");
            if (!test_bit(ABS_PRESSURE, caps.absbits))
               gpm_report(GPM_PR_WARN, "evdev: selected synaptics mode but device does not report pressure");
            if (!test_bit(EV_MSC, caps.evbits) || !test_bit(MSC_GESTURE, caps.mscbits))
               gpm_report(GPM_PR_WARN, "evdev: selected synaptics mode but device does not report gesture events");
            evdev->y_inverted = 1;     /* Synaptics always has Y inverted */
            break;
      }
   }

   return 0;
}

