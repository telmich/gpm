static int M_logimsc(Gpm_Event *state,  unsigned char *data) /* same as msc */
{
   state->buttons= (~data[0]) & 0x07;
   state->dx=      (signed char)(data[1]) + (signed char)(data[3]);
   state->dy=     -((signed char)(data[2]) + (signed char)(data[4]));
   return 0;
}

