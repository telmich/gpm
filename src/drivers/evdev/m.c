#ifdef HAVE_LINUX_INPUT_H

static int M_evdev (Gpm_Event * state, unsigned char *data)
{
   struct input_event thisevent;
   (void) memcpy (&thisevent, data, sizeof (struct input_event));
   if (thisevent.type == EV_REL) {
      if (thisevent.code == REL_X)
         state->dx = (signed char) thisevent.value;
      else if (thisevent.code == REL_Y)
         state->dy = (signed char) thisevent.value;
   } else if (thisevent.type == EV_KEY) {
      switch(thisevent.code) {
         case BTN_LEFT:    state->buttons ^= GPM_B_LEFT;    break;
         case BTN_MIDDLE:  state->buttons ^= GPM_B_MIDDLE;  break;
         case BTN_RIGHT:   state->buttons ^= GPM_B_RIGHT;   break;
         case BTN_SIDE:    state->buttons ^= GPM_B_MIDDLE;  break;
      }
   }
   return 0;
}

#endif
