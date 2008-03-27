/**
 * Writes the given data to the ps2-type mouse.
 * Checks for an ACK from each byte.
 * 
 * Returns 0 if OK, or >0 if 1 or more errors occurred.
 */
static int write_to_mouse(int fd, unsigned char *data, size_t len)
{
   int i;
   int error = 0;
   for (i = 0; i < len; i++) {
      unsigned char c;
      write(fd, &data[i], 1);
      read(fd, &c, 1);
      if (c != GPM_AUX_ACK) error++;
   }

   /* flush any left-over input */
   usleep (30000);
   tcflush (fd, TCIFLUSH);
   return(error);
}

