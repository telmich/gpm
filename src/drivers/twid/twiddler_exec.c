int twiddler_exec(char *s)
{
   int pid;

   extern struct options option;

   switch (pid = fork()) {
      case -1:
         return -1;
      case 0:
         close(0);
         close(1);
         close(2);              /* very rude! */

         open(GPM_NULL_DEV, O_RDONLY);
         open(option.consolename, O_WRONLY);
         dup(1);
         execl("/bin/sh", "sh", "-c", s, NULL);
         exit(1);               /* shouldn't happen */

      default:                 /* father: */
         return (0);
   }
}
