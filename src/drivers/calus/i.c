static Gpm_Type *I_calus(int fd, unsigned short flags,
          struct Gpm_Type *type, int argc, char **argv)
{
   if (check_no_argv(argc, argv)) return NULL;

   if ((which_mouse->opt_baud) == 1200) (which_mouse->opt_baud)=9600; /* default to 9600 */
   return I_serial(fd, flags, type, argc, argv);
}

