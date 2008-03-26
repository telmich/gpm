static int M_bm(Gpm_Event *state,  unsigned char *data) /* equal to sun */
{
   state->buttons= (~data[0]) & 0x07;
   state->dx=      (signed char)data[1];
   state->dy=     -(signed char)data[2];
   return 0;
}

