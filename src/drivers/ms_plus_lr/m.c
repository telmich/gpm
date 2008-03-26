int M_ms_plus_lr(Gpm_Event *state,  unsigned char *data)
{
   /*
    * Same as M_ms_plus but with an addition by Edmund GRIMLEY EVANS
    */
   static unsigned char prev=0;

   state->buttons= ((data[0] & 0x20) >> 3) | ((data[0] & 0x10) >> 4);
   state->dx=      (signed char)(((data[0] & 0x03) << 6) | (data[1] & 0x3F));
   state->dy=      (signed char)(((data[0] & 0x0C) << 4) | (data[2] & 0x3F));

   /* Allow motion *and* button change (Michael Plass) */

   if((state->dx==0)&& (state->dy==0) && (state->buttons==(prev&~GPM_B_MIDDLE)))
      state->buttons = prev^GPM_B_MIDDLE; /* no move or change: toggle middle */
   else
      state->buttons |= prev&GPM_B_MIDDLE;/* change: preserve middle */

   /* Allow the user to reset state of middle button by pressing
      the other two buttons at once (Edmund GRIMLEY EVANS) */

   if (!((~state->buttons)&(GPM_B_LEFT|GPM_B_RIGHT)) &&
      ((~prev)&(GPM_B_LEFT|GPM_B_RIGHT)))
      state->buttons &= ~GPM_B_MIDDLE;

   prev=state->buttons;

   return 0;
}
