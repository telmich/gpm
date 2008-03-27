static Gpm_Type *I_twid(int fd, unsigned short flags,
         struct Gpm_Type *type, int argc, char **argv)
{

   if (check_no_argv(argc, argv)) return NULL;

   if (twiddler_key_init() != 0) return NULL;
   /*
   * the twiddler is a serial mouse: just drop dtr
   * and run at 2400 (unless specified differently) 
   */
   if((which_mouse->opt_baud)==DEF_BAUD) (which_mouse->opt_baud) = 2400;
   argv[1] = "dtr"; /* argv[1] is guaranteed to be NULL (this is dirty) */
   return I_serial(fd, flags, type, argc, argv);
}

