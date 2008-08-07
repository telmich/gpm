/* return value is one if auto-repeating, 0 if not */
int twiddler_key(unsigned long message)
{
   char **table = twiddler_get_table(message);

   char *val;

   /*
    * These two are needed to avoid transmitting single keys when typing
    * chords. When the number of keys being held down decreases, data
    * is transmitted; but as soon as it increases the cycle is restarted.
    */
   static unsigned long last_message;

   static int marked;

   /*
    * The time values are needed to implement repetition of keys
    */
   static struct timeval tv1, tv2;

   static unsigned int nclick, last_pressed;

#define GET_TIME(tv) (gettimeofday(&tv, (struct timezone *)NULL))
#define DIF_TIME(t1,t2) ((t2.tv_sec -t1.tv_sec) *1000+ \
                        (t2.tv_usec-t1.tv_usec)/1000)

   if(!table)
      return 0;
   message &= 0xff;
   val = table[message];

   if((message < last_message) && !marked) {    /* ok, do it */
      marked++;                 /* don't retransmit on release */
      twiddler_use_item(table[last_message]);
      if(last_pressed != last_message) {
         nclick = 1;
         GET_TIME(tv1);
      }                         /* first click (note: this is release) */
      last_pressed = last_message;
   }
   if(message < last_message) { /* marked: just ignore */
      last_message = message;
      return 0;
   }

   /*
    * building up a chord or repeating 
    */

   if(message != last_pressed) {        /* building up */
      marked = 0;
      last_message = message;
      return 0;
   }

   /*
    * Hmmm... double click 
    */
   if(message > last_message) {
      marked = 0;               /* but don't use it */
      GET_TIME(tv2);
      nclick = 1 + (DIF_TIME(tv1, tv2) < 300);  /* if fast, counts as double */
      last_message = message;
      if(nclick == 1)
         GET_TIME(tv1);         /* maybe the next.. */
      return 1;
   }

   /*
    * so, we are repeating... 
    */
   if(nclick == 2) {
      GET_TIME(tv1);            /* compute delay */
      if(DIF_TIME(tv2, tv1) > 500)      /* held enough */
         twiddler_use_item(table[message]);
      return 1;
   }
   return 0;
}
