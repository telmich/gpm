/* standard ps2 */
static Gpm_Type *I_ps2(int fd, unsigned short flags,
        struct Gpm_Type *type, int argc, char **argv)
{
   static unsigned char s[] = { 246, 230, 244, 243, 100, 232, 3, };
   write (fd, s, sizeof (s));
   usleep (30000);
   tcflush (fd, TCIFLUSH);
   return type;
}

