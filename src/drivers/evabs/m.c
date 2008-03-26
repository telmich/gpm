#ifdef HAVE_LINUX_INPUT_H
static int M_evabs (Gpm_Event * state, unsigned char *data)
{
   struct input_event thisevent;
   (void) memcpy (&thisevent, data, sizeof (struct input_event));
   if (thisevent.type == EV_ABS) {
      if (thisevent.code == ABS_X)
         state->x = thisevent.value;
      else if (thisevent.code == ABS_Y)
         state->y = thisevent.value;
   } else if (thisevent.type == EV_KEY) {
      switch(thisevent.code) {
         case BTN_LEFT:    state->buttons ^= GPM_B_LEFT;    break;
         case BTN_MIDDLE:  state->buttons ^= GPM_B_MIDDLE;  break;
         case BTN_RIGHT:   state->buttons ^= GPM_B_RIGHT;   break;
         case BTN_SIDE:    state->buttons ^= GPM_B_MIDDLE;  break;
         case BTN_TOUCH:   state->buttons ^= GPM_B_LEFT;    break;
      }
   }
   return 0;
}
#endif /* HAVE_LINUX_INPUT_H */

