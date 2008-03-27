/* simple initialization for the gunze touchscreen */
static Gpm_Type *I_gunze(int fd, unsigned short flags,
           struct Gpm_Type *type, int argc, char **argv)
{
   struct termios tty;
   FILE *f;
   char s[80];
   int i, calibok = 0;

   #define GUNZE_CALIBRATION_FILE SYSCONFDIR "/gpm-calibration"
   /* accept a few options */
   static argv_helper optioninfo[] = {
      {"smooth",   ARGV_INT, u: {iptr: &gunze_avg}},
      {"debounce", ARGV_INT, u: {iptr: &gunze_debounce}},
      /* FIXME: add corner tapping */
      {"",       ARGV_END}
   };
   parse_argv(optioninfo, argc, argv);

   /* check that the baud rate is valid */
   if ((which_mouse->opt_baud) == DEF_BAUD) (which_mouse->opt_baud) = 19200; /* force 19200 as default */
   if ((which_mouse->opt_baud) != 9600 && (which_mouse->opt_baud) != 19200) {
      gpm_report(GPM_PR_ERR,GPM_MESS_GUNZE_WRONG_BAUD,option.progname, argv[0]);
      (which_mouse->opt_baud) = 19200;
   }
   tcgetattr(fd, &tty);
   tty.c_iflag = IGNBRK | IGNPAR;
   tty.c_oflag = 0;
   tty.c_lflag = 0;
   tty.c_line = 0;
   tty.c_cc[VTIME] = 0;
   tty.c_cc[VMIN] = 1;
   tty.c_cflag = ((which_mouse->opt_baud) == 9600 ? B9600 : B19200) |CS8|CREAD|CLOCAL|HUPCL;
   tcsetattr(fd, TCSAFLUSH, &tty);

   /* FIXME: try to find some information about the device */

   /* retrieve calibration, if not existent, use defaults (uncalib) */
   f = fopen(GUNZE_CALIBRATION_FILE, "r");
   if (f) {
      fgets(s, 80, f); /* discard the comment */
      if (fscanf(f, "%d %d %d %d", gunze_calib, gunze_calib+1,
         gunze_calib+2, gunze_calib+3) == 4)
         calibok = 1;
   /* Hmm... check */
      for (i=0; i<4; i++)
         if (gunze_calib[i] & ~1023) calibok = 0;
      if (gunze_calib[0] == gunze_calib[2]) calibok = 0;
      if (gunze_calib[1] == gunze_calib[3]) calibok = 0;
      fclose(f);
   }
   if (!calibok) {
      gpm_report(GPM_PR_ERR,GPM_MESS_GUNZE_CALIBRATE, option.progname);
      gunze_calib[0] = gunze_calib[1] = 128; /* 1/8 */
      gunze_calib[2] = gunze_calib[3] = 896; /* 7/8 */
   }
   return type;
}

