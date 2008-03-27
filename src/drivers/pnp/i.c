static Gpm_Type *I_pnp(int fd, unsigned short flags,
             struct Gpm_Type *type, int argc, char **argv)
{
   struct termios tty;

   /* accept "-o dtr", "-o rts" and "-o both" */
   if (option_modem_lines(fd, argc, argv)) return NULL;

   /*
    * Just put the device to 1200 baud. Thanks to Francois Chastrette
    * for his great help and debugging with his own pnp device.
    */
   tcgetattr(fd, &tty);

   tty.c_iflag = IGNBRK | IGNPAR;
   tty.c_oflag = 0;
   tty.c_lflag = 0;
   tty.c_line = 0;
   tty.c_cc[VTIME] = 0;
   tty.c_cc[VMIN] = 1;
   tty.c_cflag = flags | B1200;
   tcsetattr(fd, TCSAFLUSH, &tty); /* set parameters */

   /*
    * Don't read the silly initialization string. I don't want to see
    * the vendor name: it is only propaganda, with no information.
    */

   return type;
}

