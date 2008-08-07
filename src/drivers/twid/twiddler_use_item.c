/* And this uses the item to push keys */
static inline int twiddler_use_item(char *item)
{
   int fd = open_console(O_WRONLY);

   int i, retval = 0;

   unsigned char pushthis, unblank = 4; /* 4 == TIOCLINUX unblank */

   /*
    * a special function 
    */
   /*
    * a single byte 
    */
   if(((unsigned long) item & 0xff) == (unsigned long) item) {
      pushthis = (unsigned long) item & 0xff;
      retval = ioctl(fd, TIOCSTI, &pushthis);
   } else if(i = (struct twiddler_active_fun *) item - twiddler_active_funs,
             i >= 0 && i < active_fun_nr)
      twiddler_do_fun(i);
   else                         /* a string */
      for(; *item != '\0' && retval == 0; item++)
         retval = ioctl(fd, TIOCSTI, item);

   ioctl(fd, TIOCLINUX, &unblank);
   if(retval)
      gpm_report(GPM_PR_ERR, GPM_MESS_IOCTL_TIOCSTI, option.progname,
                 strerror(errno));
   close(fd);
   return retval;
}

