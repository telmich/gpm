static Gpm_Type *I_mtouch(int fd, unsigned short flags,
           struct Gpm_Type *type, int argc, char **argv)
{
   struct termios tty;

   /* Set speed to 9600bps (copied from I_summa, above :) */
   tcgetattr(fd, &tty);
   tty.c_iflag = IGNBRK | IGNPAR;
   tty.c_oflag = 0;
   tty.c_lflag = 0;
   tty.c_line = 0;
   tty.c_cc[VTIME] = 0;
   tty.c_cc[VMIN] = 1;
   tty.c_cflag = B9600|CS8|CREAD|CLOCAL|HUPCL;
   tcsetattr(fd, TCSAFLUSH, &tty);


   /* Turn it to "format tablet" and "mode stream" */
   write(fd,"\001MS\r\n\001FT\r\n",10);

   return type;
}

