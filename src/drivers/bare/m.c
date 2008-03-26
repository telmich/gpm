static int M_bare(Gpm_Event *state,  unsigned char *data)
{
   /* a bare ms protocol */
   state->buttons= ((data[0] & 0x20) >> 3) | ((data[0] & 0x10) >> 4);
   state->dx=      (signed char)(((data[0] & 0x03) << 6) | (data[1] & 0x3F));
   state->dy=      (signed char)(((data[0] & 0x0C) << 4) | (data[2] & 0x3F));
   return 0;
}

