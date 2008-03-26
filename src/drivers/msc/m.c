static int M_msc(Gpm_Event *state,  unsigned char *data)
{
   state->buttons= (~data[0]) & 0x07;
   state->dx=      (signed char)(data[1]) + (signed char)(data[3]);
   state->dy=     -((signed char)(data[2]) + (signed char)(data[4]));
   return 0;
}

