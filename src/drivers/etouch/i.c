/* simple initialization for the elo touchscreen */
static Gpm_Type *I_etouch(int fd, unsigned short flags,
           struct Gpm_Type *type, int argc, char **argv)
{
  struct termios tty;
  FILE *f;
  char s[80];
  int i, calibok = 0;

  /* Calibration config file (copied from I_gunze, below :) */
  #define ELO_CALIBRATION_FILE SYSCONFDIR "/gpm-calibration"
   /* accept a few options */
   static argv_helper optioninfo[] = {
         {"clickontouch",  ARGV_BOOL, u: {iptr: &elo_click_ontouch}, value: !0},
         {"",       ARGV_END}
   };
   parse_argv(optioninfo, argc, argv);

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

  /* retrieve calibration, if not existent, use defaults (uncalib) */
  f = fopen(ELO_CALIBRATION_FILE, "r");
  if (f) {
   fgets(s, 80, f); /* discard the comment */
   if (fscanf(f, "%d %d %d %d", gunze_calib, gunze_calib+1,
         gunze_calib+2, gunze_calib+3) == 4)
       calibok = 1;
   /* Hmm... check */
   for (i=0; i<4; i++)
       if ((gunze_calib[i] & 0xfff) != gunze_calib[i]) calibok = 0;
   if (gunze_calib[0] == gunze_calib[2]) calibok = 0;
   if (gunze_calib[1] == gunze_calib[4]) calibok = 0;
   fclose(f);
  }
  if (!calibok) {
        gpm_report(GPM_PR_ERR,GPM_MESS_ELO_CALIBRATE, option.progname, ELO_CALIBRATION_FILE);
        /* "%s: etouch: calibration file %s absent or invalid, using defaults" */
   gunze_calib[0] = gunze_calib[1] = 0x010; /* 1/16 */
   gunze_calib[2] = gunze_calib[3] = 0xff0; /* 15/16 */
  }
  return type;
}

