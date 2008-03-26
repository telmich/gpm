static int M_netmouse(Gpm_Event *state,  unsigned char *data)
{
   /* Avoid these beasts if you can.  They connect to normal PS/2 port,
    * but their protocol is one byte longer... So if you have notebook
    * (like me) with internal PS/2 mouse, it will not work
    * together. They have four buttons, but two middle buttons can not
    * be pressed simultaneously, and two middle buttons do not send
    * 'up' events (however, they autorepeat...)

    * Still, you might want to run this mouse in plain PS/2 mode -
    * where it behaves correctly except that middle 2 buttons do
    * nothing.

    * Protocol is
    * 3 bytes like normal PS/2
    * 4th byte: 0xff button 'down', 0x01 button 'up'
    * [this is so braindamaged that it *must* be some kind of
    * compatibility glue...]

    * Pavel Machek <pavel@ucw.cz>
    */

   state->buttons=
      !!(data[0]&1) * GPM_B_LEFT +
      !!(data[0]&2) * GPM_B_RIGHT +
      !!(data[3]) * GPM_B_MIDDLE;

   if(data[1] != 0)
      state->dx=   (data[0] & 0x10) ? data[1]-256 : data[1];
   else
      state->dx = 0;
   if(data[2] != 0)
      state->dy= -((data[0] & 0x20) ? data[2]-256 : data[2]);
   else
      state->dy = 0;
   return 0;
}

