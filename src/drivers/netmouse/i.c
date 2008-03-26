static Gpm_Type *I_netmouse(int fd, unsigned short flags,
         struct Gpm_Type *type, int argc, char **argv)
{
   unsigned char magic[6] = { 0xe8, 0x03, 0xe6, 0xe6, 0xe6, 0xe9 };
   int i;

   if (check_no_argv(argc, argv)) return NULL;
   for (i=0; i<6; i++) {
      unsigned char c = 0;
      write( fd, magic+i, 1 );
      read( fd, &c, 1 );
      if (c != 0xfa) {
         gpm_report(GPM_PR_ERR,GPM_MESS_NETM_NO_ACK,c);
         return NULL;
      }
   }
   {
      unsigned char rep[3] = { 0, 0, 0 };
      read( fd, rep, 1 );
      read( fd, rep+1, 1 );
      read( fd, rep+2, 1 );
      if (rep[0] || (rep[1] != 0x33) || (rep[2] != 0x55)) {
         gpm_report(GPM_PR_ERR,GPM_MESS_NETM_INV_MAGIC, rep[0], rep[1], rep[2]);
         return NULL;
      }
   }
   return type;
}

