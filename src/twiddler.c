/*
 * twiddler.c - support for the twiddler keyboard
 *
 * Copyright 1998   rubini@linux.it (Alessandro Rubini)
 * Changed 04/23/2001 nico@schottelius.org (Nico Schottelius)
 *  --> removed hard wired dev names. Instead used symbolic constants and
 *     generated option.consolename
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

/* TODO:
     resolution change (mice.c)
     switch up/down (mice.c)
     manage X....
     modifiers
 */

/* Done:
      two cfg files
      mouse buttons
      unblank
      special keys...
      meta keys
      auto-repeat: double press plus hold...
      README
*/
      

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <linux/vt.h>

#include "headers/gpm.h"
#include "headers/gpmInt.h"
#include "headers/message.h"
#include "headers/twiddler.h"

#include "headers/daemon.h"

/* 
 * Each table is made up of 256 entries, as these are the possible chords
 * Then, there is one table for each modifier (two of them are not allowed).
 * Each entry is a pointer: a "low" pointer represents a single byte, true
 * pointers represent strings. This is not very clean, but it works well.
 */

char *twiddler_table[7][256];

/* This maps modifiers to tables */
struct twiddler_map_struct {
   unsigned long modifiers;
   char *keyword;
   char **table;
} twiddler_map[] = {
   { TW_MOD_0, "",   twiddler_table[0]},
   { TW_MOD_S, "Shift", twiddler_table[1]},
   { TW_MOD_N, "Numeric",  twiddler_table[2]},
   { TW_MOD_F, "Function", twiddler_table[3]},
   { TW_MOD_C, "Control",  twiddler_table[4]},
   { TW_MOD_C, "Ctrl",    twiddler_table[4]},
   { TW_MOD_A, "Alt",     twiddler_table[5]}, 
   { TW_MOD_A, "Meta",    twiddler_table[5]}, 
/* This is different!! */
   { TW_MOD_C | TW_MOD_S, "Ctrl+Shift",    twiddler_table[6]}, 
   { TW_MOD_C | TW_MOD_S, "Shift+Ctrl",    twiddler_table[6]}, 
   { 0,       NULL,      NULL}
};

/* Convert special keynames to strings */
struct twiddler_f_struct {
   char *instring;
   char *outstring;
} twiddler_f[] = {
   {"F1",    "\033[[A" },
   {"F2",    "\033[[B" },
   {"F3",    "\033[[C" },
   {"F4",    "\033[[D" },
   {"F5",    "\033[[E" },
   {"F6",    "\033[17~" },
   {"F7",    "\033[18~" },
   {"F8",    "\033[19~" },
   {"F9",    "\033[20~" },
   {"F10",   "\033[21~" },
   {"F11",   "\033[23~" },
   {"F12",   "\033[24~" },
   {"F13",   "\033[25~" },
   {"F14",   "\033[26~" },
   {"F15",   "\033[28~" },
   {"F16",   "\033[29~" },
   {"F17",   "\033[31~" },
   {"F18",   "\033[32~" },
   {"F19",   "\033[33~" },
   {"F20",   "\033[34~" },
   {"Find",   "\033[1~" },
   {"Insert", "\033[2~" },
   {"Remove", "\033[3~" },
   {"Select", "\033[4~" },
   {"Prior",  "\033[5~" },
   {"Next",   "\033[6~" },
   {"Macro",  "\033[M" },
   {"Pause",  "\033[P" },

   {"Up",    "\033[A" },
   {"Down",   "\033[B" },
   {"Left",   "\033[D" },
   {"Right",  "\033[C" },
       
   {NULL, NULL }
};

/* Convert special keynames to functions */
struct twiddler_fun_struct {
   char *name;
   int (*fun)(char *string);
};


/*===================================================================*/
/*              This part deals with pushing keys                  */

/* Function support:
     if a chord maps to a function, we need two pointers: one to the
     function and one to its argument. Therefore we must keep an array
     that keeps both pointers, and the pointer in the table is just
     an handle to this pair of pointers. The table has a maximum len
*/

#define TWIDDLER_MAX_ACTIVE_FUNS 128

/* These are the special functions that perform special actions */
int twiddler_console(char *s)
{
   int consolenr = atoi(s); /* atoi never fails :) */
   int fd;
   if (consolenr == 0) return 0; /* nothing to do */

   fd=open_console(O_RDONLY);
   if (fd < 0) return -1;
   if (ioctl(fd, VT_ACTIVATE, consolenr)<0) {close(fd); return -1;}
/*if (ioctl(fd, VT_WAITACTIVE, consolenr)<0) {close(fd); return -1;} */
   close(fd);
   return 0;
}

int twiddler_exec(char *s)
{
   int pid;
   extern struct options option;
   switch(pid=fork()) {
      case -1: return -1;
      case 0:
         close(0);
         close(1);
         close(2); /* very rude! */
      
         open(GPM_NULL_DEV,O_RDONLY);
         open(option.consolename,O_WRONLY);
         dup(1);
         execl("/bin/sh", "sh", "-c", s, NULL);
         exit(1); /* shouldn't happen */

   default: /* father: */
         return(0);
   }
}

/* The functions and their name */
struct twiddler_fun_struct twiddler_functions[] = {
   { "Console",  twiddler_console },
   { "Exec",    twiddler_exec },
   { NULL, NULL}
};

/* The registered functions */
struct twiddler_active_fun {
   int (*fun)(char *s);
   char *arg;
   } twiddler_active_funs[TWIDDLER_MAX_ACTIVE_FUNS];
static int active_fun_nr = 0;


int twiddler_do_fun(int i)
{
   twiddler_active_funs[i].fun(twiddler_active_funs[i].arg);
   return 0;
}

/*
 * Ok, from now on it's normal handling
 */

/* This returns the table to use */
static inline char **twiddler_get_table(unsigned long message)
{
   unsigned long mod = message & TW_ANY_MOD;
   struct twiddler_map_struct *ptr;

   for (ptr = twiddler_map; ptr->table; ptr++)
      if (ptr->modifiers == mod) return ptr->table;
   return NULL;
}

/* And this uses the item to push keys */
static inline int twiddler_use_item(char *item)
{
   int fd = open_console(O_WRONLY);
   int i, retval = 0;
   unsigned char pushthis, unblank=4; /* 4 == TIOCLINUX unblank */

 /* a special function */ 
   /* a single byte */
   if (((unsigned long)item & 0xff) == (unsigned long)item) {
      pushthis = (unsigned long)item & 0xff;
      retval = ioctl(fd,TIOCSTI,&pushthis);
   } else if(i = (struct twiddler_active_fun *)item - twiddler_active_funs,
                  i>=0 && i < active_fun_nr)
      twiddler_do_fun(i);
   else /* a string */
      for (; *item!='\0' && retval==0; item++) retval = ioctl(fd,TIOCSTI,item);

   ioctl(fd,TIOCLINUX,&unblank);
   if (retval)
   gpm_report(GPM_PR_ERR,GPM_MESS_IOCTL_TIOCSTI, option.progname, strerror(errno));
   close(fd);
   return retval;
}

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
   static int last_message;
   static int marked;
  /*
   * The time values are needed to implement repetition of keys
   */
   static struct timeval tv1, tv2;
   static int nclick, last_pressed;
#define GET_TIME(tv) (gettimeofday(&tv, (struct timezone *)NULL))
#define DIF_TIME(t1,t2) ((t2.tv_sec -t1.tv_sec) *1000+ \
                        (t2.tv_usec-t1.tv_usec)/1000)

   if (!table) return 0;
   message &= 0xff;
   val = table[message];

   if ((message < last_message) && !marked) { /* ok, do it */
      marked++; /* don't retransmit on release */
      twiddler_use_item(table[last_message]);
      if (last_pressed != last_message) {
         nclick=1; GET_TIME(tv1);
      } /* first click (note: this is release) */
      last_pressed = last_message;
   }
   if (message < last_message) { /* marked: just ignore */
      last_message = message;
      return 0;
   }

  /* building up a chord or repeating */

   if (message != last_pressed) { /* building up */
      marked=0;
      last_message = message;
      return 0;
   }

  /*  Hmmm... double click */
   if (message > last_message) {
      marked = 0; /* but don't use it */
      GET_TIME(tv2);
      nclick = 1+ (DIF_TIME(tv1,tv2) < 300); /* if fast, counts as double */
      last_message = message;
      if (nclick==1) GET_TIME(tv1); /* maybe the next.. */
      return 1;
   }

  /* so, we are repeating... */
   if (nclick == 2) {
      GET_TIME(tv1); /* compute delay */
      if (DIF_TIME(tv2,tv1) > 500) /* held enough */
         twiddler_use_item(table[message]);
      return 1;
   }
   return 0;
}

/*===================================================================*/
/*          This part deals with reading the cfg file(s)           */

/* retrieve the right table according to the modifier string */
char **twiddler_mod_to_table(char *mod)
{
   struct twiddler_map_struct *ptr;
   int len = strlen(mod);
   if (len == 0) return twiddler_map->table;

   for (ptr = twiddler_map; ptr->table; ++ptr) {
      if (!strncasecmp(mod,ptr->keyword,len))
      return ptr->table;
   }
   return NULL;
}

/* Convert the "M00L"-type string to an integer */
int twiddler_chord_to_int(char *chord)
{
   char *convert = "0LMR"; /* 0123 */
   char *tmp;
   int result = 0;
   int shift = 0;

   if (strlen(chord)!=4) return -1;

   while (*chord) {
      if (!(tmp = strchr(convert, *chord))) return -1;
      result |= (tmp-convert)<<shift;
      chord++;
      shift += 2;
   }
   return result;
}

/* Convert an escape sequence to a single byte */
int twiddler_escape_sequence(char *s, int *len)
{
   int res = 0;
   *len=0;
   if (isdigit(*s)) { /* octal */
      while (isdigit(*s)) {
         res *= 8;
         res += *s-'0';
         s++;
         (*len)++;
         if (*len == 3) return res;
      }
      return res;
   }

   *len=1;
   switch(*s) {
      case 'n': return '\n';
      case 'r': return '\r';
      case 'e': return '\e';
      case 't': return '\t';
      case 'a': return '\a';
      case 'b': return '\b';
      default: return *s;

      /* a case statement after default: ???? */
      case 'x': /* hex */
         (*len)++; s++;
         if (isdigit(*s)) res = *s-'0';
         else res          = tolower(*s)-'a'+10;
         s++;
         if (isxdigit(*s)) {
            (*len)++;
            res *= 16;
            if (isdigit(*s)) res += *s-'0';
            else res          += tolower(*s)-'a'+10;
         }
         return res;
   }
} 

/*
 * Convert the rest to a string or a byte
 * the special return value "s" signals an error
 */
char *twiddler_rest_to_value(char *s)
{
   char *ptr;
   static char buf[64];
   int ibuf = 0, len;
   struct twiddler_f_struct *sptr;
   struct twiddler_fun_struct *fptr;

   /* look for the value */
   ptr = strchr(s,'=');
   if (!ptr) return s;
   ptr++;
   while (isspace(*ptr)) ptr++;
   if (!*ptr) return s;

   /*
    * now see what the rest is: accept a single char,
    * standalone escape sequences or strings (with escape seq.)
    * Whenever the string is not quoted, try to interpret it
    */
   if (*ptr == '"') { /* string... */
      ptr++;
      while (*ptr != '"') {
         if (*ptr == '\\') {
            ptr++;
            buf[ibuf++] = twiddler_escape_sequence(ptr, &len);
            ptr += len;
         }
         else
            buf[ibuf++]=*(ptr++);
      }
      buf[ibuf]='\0';
      return strdup(buf);
   }
   if (*ptr == '\\')
      return (char *)twiddler_escape_sequence(ptr+1, &len /* unused */);

   if (strlen(ptr)==1) return ((char *)((unsigned long)*ptr & 0xFF));

  /* look for special string */
   for (sptr = twiddler_f; sptr->instring; sptr++)
      if (!strcmp(ptr,sptr->instring)) return sptr->outstring;

   /* finally, look for special functions */
   for (fptr = twiddler_functions; fptr->name; fptr++)
      if (!strncasecmp(fptr->name,ptr,strlen(fptr->name))) {
         if (active_fun_nr == TWIDDLER_MAX_ACTIVE_FUNS) {
            gpm_report(GPM_PR_ERR,GPM_MESS_TOO_MANY_SPECIAL, option.progname,
                            TWIDDLER_MAX_ACTIVE_FUNS);
            return s;
         }
         twiddler_active_funs[active_fun_nr].fun = fptr->fun;
         twiddler_active_funs[active_fun_nr].arg=strdup(ptr+strlen(fptr->name));
         return (char *)(twiddler_active_funs + active_fun_nr++);
      }

   return s; /* error */
}

int twiddler_key_init(void)
{
   FILE *f;
   char *files[]={TW_SYSTEM_FILE, TW_CUSTOM_FILE, NULL};
   int fileindex=0;
   char s[128], buf[64]; /* buf is for the string */
   char mod[64], chord[64], *value;
   int index, lineno=0, errcount=0;
   char **table;
   static int initcount=0;

   if (initcount) return 0; /* do it only once */
   initcount++;

   if (!(f = fopen(TW_SYSTEM_FILE,"r"))) {
      gpm_report(GPM_PR_ERR,GPM_MESS_DOUBLE_S,TW_SYSTEM_FILE,strerror(errno));
      return -1;
   }

  /* ok, go on reading all the files in files[]*/
  while (files[fileindex]) {
      while (fgets(s,128,f)) {
         lineno++;

         /* trim newline and blanks, if any */
         if (s[strlen(s)-1]=='\n') s[strlen(s)-1] = '\0';
         while (isspace(s[strlen(s)-1])) s[strlen(s)-1] = '\0';

         if (s[0]=='\0' || s[0]=='#') continue; /* ignore comment or empty */

         if (sscanf(s,"%s = %s",chord,buf)==2) /* M00L = anything */
            mod[0]='\0'; /* no modifiers */
         else if (sscanf(s, "%s %s = %s", mod, chord, buf)==3) /*Mod M00L =k */;
         else if (sscanf(s, "%s", buf)!= 0) {
            gpm_report(GPM_PR_ERR,GPM_MESS_SYNTAX_1,option.progname,TW_SYSTEM_FILE
                                                   ,lineno);
	         errcount++;
	         continue;
	      }

         /* Ok, here we are: now check the parts */
         if (!(table = twiddler_mod_to_table(mod))) {
            gpm_report(GPM_PR_ERR,GPM_MESS_UNKNOWN_MOD_1,option.progname,TW_SYSTEM_FILE
                                                        ,lineno, mod);
	         errcount++;
	         continue;
	      }
         if ((index = twiddler_chord_to_int(chord)) <= 0) {
            gpm_report(GPM_PR_ERR,GPM_MESS_INCORRECT_COORDS,option.progname,
                                      TW_SYSTEM_FILE,lineno, chord);
	         errcount++;
	         continue;
	      }
         if ((value = twiddler_rest_to_value(s))==s) {
	         gpm_report(GPM_PR_ERR,GPM_MESS_INCORRECT_LINE,option.progname,TW_SYSTEM_FILE
                                                         ,lineno,s);
	         errcount++;
	         continue;
	      }

         if (table[index]) {
	         gpm_report(GPM_PR_ERR,GPM_MESS_REDEF_COORDS,option.progname,TW_SYSTEM_FILE,
                                          lineno, mod, *mod ? " " : "", chord);
	      }
         /* all done */
         if (value)
	         table[index]=value;
         else
	         table[index]=""; /* the empty string represents the nul byte */
      }  /* fgets */
      fclose(f);
      while (files[++fileindex]) /* next file, optional */
         if ( (f=fopen(files[fileindex],"r")) ) break;
   }
   return errcount;
}
