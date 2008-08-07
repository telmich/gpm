
/*
 * Support for the twiddler keyboard
 *
 * Copyright 1998          Alessandro Rubini (rubini at linux.it)
 * Copyright 2001 - 2008   Nico Schottelius (nico-gpm2008 at schottelius.org)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 ********/

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
