/* ps2 */
int R_ps2(Gpm_Event *state, int fd)
{
   signed char buffer[3];

   buffer[0]=((state->buttons & GPM_B_LEFT  ) > 0)*1 +
             ((state->buttons & GPM_B_RIGHT ) > 0)*2 +
             ((state->buttons & GPM_B_MIDDLE) > 0)*4;
   buffer[0] |= 8 +
             ((state->dx < 0) ? 0x10 : 0) +
             ((state->dy > 0) ? 0x20 : 0);
   buffer[1] = limit_delta(state->dx, -128, 127);
   buffer[2] = limit_delta(-state->dy, -128, 127);
   return write(fd,buffer,3);
}

