/* Convert an escape sequence to a single byte */
int twiddler_escape_sequence(char *s, int *len)
{
   int res = 0;

   *len = 0;
   if(isdigit(*s)) {            /* octal */
      while(isdigit(*s)) {
         res *= 8;
         res += *s - '0';
         s++;
         (*len)++;
         if(*len == 3)
            return res;
      }
      return res;
   }

   *len = 1;
   switch (*s) {
      case 'n':
         return '\n';
      case 'r':
         return '\r';
      case 'e':
         return '\e';
      case 't':
         return '\t';
      case 'a':
         return '\a';
      case 'b':
         return '\b';
      default:
         return *s;

         /*
          * a case statement after default: ???? 
          */
      case 'x':                /* hex */
         (*len)++;
         s++;
         if(isdigit(*s))
            res = *s - '0';
         else
            res = tolower(*s) - 'a' + 10;
         s++;
         if(isxdigit(*s)) {
            (*len)++;
            res *= 16;
            if(isdigit(*s))
               res += *s - '0';
            else
               res += tolower(*s) - 'a' + 10;
         }
         return res;
   }
}
