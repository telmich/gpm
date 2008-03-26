static int setspeed(int fd,int old,int new,int needtowrite,unsigned short flags)
{
   struct termios tty;
   char *c;

   tcgetattr(fd, &tty);

   tty.c_iflag = IGNBRK | IGNPAR;
   tty.c_oflag = 0;
   tty.c_lflag = 0;
   tty.c_line = 0;
   tty.c_cc[VTIME] = 0;
   tty.c_cc[VMIN] = 1;

   switch (old) {
      case 19200: tty.c_cflag = flags | B19200; break;
      case  9600: tty.c_cflag = flags |  B9600; break;
      case  4800: tty.c_cflag = flags |  B4800; break;
      case  2400: tty.c_cflag = flags |  B2400; break;
      case  1200:
      default:    tty.c_cflag = flags | B1200; break;
   }

   tcsetattr(fd, TCSAFLUSH, &tty);

   switch (new) {
      case 19200: c = "*r";  tty.c_cflag = flags | B19200; break;
      case  9600: c = "*q";  tty.c_cflag = flags |  B9600; break;
      case  4800: c = "*p";  tty.c_cflag = flags |  B4800; break;
      case  2400: c = "*o";  tty.c_cflag = flags |  B2400; break;
      case  1200:
      default:    c = "*n";  tty.c_cflag = flags | B1200; break;
   }

   if (needtowrite) write(fd, c, 2);
   usleep(100000);
   tcsetattr(fd, TCSAFLUSH, &tty);
   return 0;
}

