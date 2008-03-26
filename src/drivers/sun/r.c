static int R_sun(Gpm_Event *state, int fd)
{
  signed char buffer[3];

  buffer[0]= (state->buttons ^ 0x07) | 0x80;
  buffer[1]= state->dx;
  buffer[2]= -(state->dy);
  return write(fd,buffer,3);
}

