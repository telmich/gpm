/* These are the special functions that perform special actions */
int twiddler_console(char *s)
{
   int consolenr = atoi(s);     /* atoi never fails :) */

   int fd;

   if(consolenr == 0)
      return 0;                 /* nothing to do */

   fd = open_console(O_RDONLY);
   if(fd < 0)
      return -1;
   if(ioctl(fd, VT_ACTIVATE, consolenr) < 0) {
      close(fd);
      return -1;
   }

/*if (ioctl(fd, VT_WAITACTIVE, consolenr)<0) {close(fd); return -1;} */
   close(fd);
   return 0;
}
