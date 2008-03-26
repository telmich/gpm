static int M_sun(Gpm_Event *state,  unsigned char *data)
{
   state->buttons= (~data[0]) & 0x07;
   state->dx=      (signed char)(data[1]);
   state->dy=     -(signed char)(data[2]);
   return 0;
}

