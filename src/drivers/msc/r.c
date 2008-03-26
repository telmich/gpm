static int R_msc(Gpm_Event *state, int fd)
{
   signed char buffer[5];
   int dx, dy;

   /* sluggish... */
   buffer[0]=(state->buttons ^ 0x07) | 0x80;
   dx = limit_delta(state->dx, -256, 254);
   buffer[3] =  state->dx - (buffer[1] = state->dx/2); /* Markus */
   dy = limit_delta(state->dy, -256, 254);
   buffer[4] = -state->dy - (buffer[2] = -state->dy/2);
   return write(fd,buffer,5);

}

