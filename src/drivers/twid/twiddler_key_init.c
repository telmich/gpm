int twiddler_key_init(void)
{
   FILE *f;
   char *files[] = { TW_SYSTEM_FILE, TW_CUSTOM_FILE, NULL };
   int fileindex = 0;

   char s[128], buf[64];        /* buf is for the string */

   char mod[64], chord[64], *value;

   int index, lineno = 0, errcount = 0;

   char **table;

   static int initcount = 0;

   if(initcount)
      return 0;                 /* do it only once */
   initcount++;

   if(!(f = fopen(TW_SYSTEM_FILE, "r"))) {
      gpm_report(GPM_PR_ERR, GPM_MESS_DOUBLE_S, TW_SYSTEM_FILE,
                 strerror(errno));
      return -1;
   }

   /*
    * ok, go on reading all the files in files[]
    */
   while(files[fileindex]) {
      while(fgets(s, 128, f)) {
         lineno++;

         /*
          * trim newline and blanks, if any 
          */
         if(s[strlen(s) - 1] == '\n')
            s[strlen(s) - 1] = '\0';
         while(isspace(s[strlen(s) - 1]))
            s[strlen(s) - 1] = '\0';

         if(s[0] == '\0' || s[0] == '#')
            continue;           /* ignore comment or empty */

         if(sscanf(s, "%s = %s", chord, buf) == 2)      /* M00L = anything */
            mod[0] = '\0';      /* no modifiers */
         else if(sscanf(s, "%s %s = %s", mod, chord, buf) == 3) /* Mod M00L =k */
            ;
         else if(sscanf(s, "%s", buf) != 0) {
            gpm_report(GPM_PR_ERR, GPM_MESS_SYNTAX_1, option.progname,
                       TW_SYSTEM_FILE, lineno);
            errcount++;
            continue;
         }

         /*
          * Ok, here we are: now check the parts 
          */
         if(!(table = twiddler_mod_to_table(mod))) {
            gpm_report(GPM_PR_ERR, GPM_MESS_UNKNOWN_MOD_1, option.progname,
                       TW_SYSTEM_FILE, lineno, mod);
            errcount++;
            continue;
         }
         if((index = twiddler_chord_to_int(chord)) <= 0) {
            gpm_report(GPM_PR_ERR, GPM_MESS_INCORRECT_COORDS, option.progname,
                       TW_SYSTEM_FILE, lineno, chord);
            errcount++;
            continue;
         }
         if((value = twiddler_rest_to_value(s)) == s) {
            gpm_report(GPM_PR_ERR, GPM_MESS_INCORRECT_LINE, option.progname,
                       TW_SYSTEM_FILE, lineno, s);
            errcount++;
            continue;
         }

         if(table[index]) {
            gpm_report(GPM_PR_ERR, GPM_MESS_REDEF_COORDS, option.progname,
                       TW_SYSTEM_FILE, lineno, mod, " ", chord);
         }
         /*
          * all done 
          */
         if(value)
            table[index] = value;
         else
            table[index] = "";  /* the empty string represents the nul byte */
      }                         /* fgets */
      fclose(f);
      while(files[++fileindex]) /* next file, optional */
         if((f = fopen(files[fileindex], "r")))
            break;
   }
   return errcount;
}
