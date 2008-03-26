static int M_mman(Gpm_Event *state,  unsigned char *data)
{
   /*
    * the damned MouseMan has 3/4 bytes packets. The extra byte 
    * is only there if the middle button is active.
    * I get the extra byte as a packet with magic numbers in it.
    * and then switch to 4-byte mode.
    */
    static unsigned char prev=0;
    static Gpm_Type *mytype=mice; /* it is the first */
    unsigned char b = (*data>>4);

   if(data[1]==GPM_EXTRA_MAGIC_1 && data[2]==GPM_EXTRA_MAGIC_2) {
      /* got unexpected fourth byte */
      if (b > 0x3) return -1;  /* just a sanity check */
      //if ((b=(*data>>4)) > 0x3) return -1;  /* just a sanity check */
      state->dx=state->dy=0;

      mytype->packetlen=4;
      mytype->getextra=0;
   }
   else {
      /* got 3/4, as expected */

      /* motion is independent of packetlen... */
      state->dx= (signed char)(((data[0] & 0x03) << 6) | (data[1] & 0x3F));
      state->dy= (signed char)(((data[0] & 0x0C) << 4) | (data[2] & 0x3F));

      prev= ((data[0] & 0x20) >> 3) | ((data[0] & 0x10) >> 4);
      if (mytype->packetlen==4) b=data[3]>>4;
   }

   if(mytype->packetlen==4) {
      if(b == 0) {
         mytype->packetlen=3;
         mytype->getextra=1;
      } else {
         if (b & 0x2) prev |= GPM_B_MIDDLE;
         if (b & 0x1) prev |= (which_mouse->opt_glidepoint_tap);
      }
   }
   state->buttons=prev;

   /* This "chord-middle" behaviour was reported by David A. van Leeuwen */
   if (((prev^state->buttons) & GPM_B_BOTH)==GPM_B_BOTH )
      state->buttons = state->buttons ? GPM_B_MIDDLE : 0;
   prev=state->buttons;

   return 0;
}

