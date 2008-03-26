static int R_imps2(Gpm_Event *state, int fd)
{
   signed char buffer[4];
   int dx, dy;

   dx = limit_delta(state->dx, -256, 255);
   dy = limit_delta(state->dy, -256, 255);

   buffer[0] = 8 |
      (state->buttons & GPM_B_LEFT ? 1 : 0) |
      (state->buttons & GPM_B_MIDDLE ? 4 : 0) |
      (state->buttons & GPM_B_RIGHT ? 2 : 0) |
      (dx < 0 ? 0x10 : 0) |
      (dy > 0 ? 0x20 : 0);
   buffer[1] = dx & 0xFF;
   buffer[2] = (-dy) & 0xFF;
   if (state->wdy > 0) buffer[3] = 0xff;
   if (state->wdy < 0) buffer[3] = 0x01;
   if (state->wdx > 0) buffer[3] = 0xfe;
   if (state->wdx < 0) buffer[3] = 0x02;

   return write(fd,buffer,4);

}

