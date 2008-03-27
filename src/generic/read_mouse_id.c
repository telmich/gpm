/*
 * Sends the SEND_ID command to the ps2-type mouse.
 * Return one of GPM_AUX_ID_...
 */
static int read_mouse_id(int fd)
{
   unsigned char c = GPM_AUX_SEND_ID;
   unsigned char id;

   write(fd, &c, 1);
   read(fd, &c, 1);
   if (c != GPM_AUX_ACK) {
      return(GPM_AUX_ID_ERROR);
   }
   read(fd, &id, 1);

   return(id);
}

