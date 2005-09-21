/*
 * gpm-root.y - a default-handler for mouse events (gpm-Linux)
 *
 * Copyright 1994,1995   rubini@linux.it (Alessandro Rubini)
 * Copyright (C) 1998    Ian Zimmerman <itz@rahul.net>
 *
 * Tue,  5 Jan 1999 23:16:45 +0000, modified by James Troup <james@nocrew.org>:
 * (get_winsize): use /dev/tty0 not /dev/console.
 * gpm-root.y (f.debug): disable undocumented f.debug function because it uses a
 * file in /tmp in a fashion which invites symlink abuse.
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
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 ********/

%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syslog.h>
#include <signal.h>         /* sigaction() */
#include <pwd.h>            /* pwd entries */
#include <grp.h>            /* initgroups() */
#include <sys/kd.h>         /* KDGETMODE */
#include <sys/stat.h>       /* fstat() */
#include <sys/utsname.h>    /* uname() */
#include <termios.h>        /* winsize */
#include <linux/limits.h>   /* OPEN_MAX */
#include <linux/vt.h>       /* VT_ACTIVATE */
#include <linux/keyboard.h> /* K_SHIFT */
#include <utmp.h>         
#include <endian.h>

#ifdef HAVE_SYS_SYSMACROS_H
#include <sys/sysmacros.h>
#else
#define major(dev) (((unsigned) (dev))>>8)
#define minor(dev) ((dev)&0xff)
#endif


#define GPM_NULL_DEV "/dev/null"

#ifdef HAVE_LINUX_MAJOR_H
#include <linux/major.h>
#else
#define VCS_MAJOR	7
#endif

#define MAX_NR_USER_CONSOLES 63 /* <linux/tty.h> needs __KERNEL__ */

#include "headers/message.h"
#include "headers/gpm.h"

#ifdef DEBUG
#define YYDEBUG 1
#else
#undef YYDEBUG
#endif

#define USER_CFG   ".gpm-root"
#define SYSTEM_CFG SYSCONFDIR "/gpm-root.conf"

#define DEFAULT_FORE 7
#define DEFAULT_BACK 0
#define DEFAULT_BORD 7
#define DEFAULT_HEAD 7

/* These macros are useful to avoid curses. The program is unportable anyway */
#define GOTOXY(f,x,y)   fprintf(f,"\x1B[%03i;%03iH",y,x)
#define FORECOLOR(f,c)  fprintf(f,"\x1B[%i;3%cm",(c)&8?1:22,colLut[(c)&7]+'0') 
#define BACKCOLOR(f,c)  fprintf(f,"\x1B[4%cm",colLut[(c)&7]+'0') 

/* These defines are ugly hacks but work */
#define ULCORNER 0xc9
#define URCORNER 0xbb
#define LLCORNER 0xc8 
#define LRCORNER 0xbc
#define HORLINE  0xcd
#define VERLINE  0xba

int colLut[]={0,4,2,6,1,5,3,7};

char *prgname;
char *consolename;
int run_status  = GPM_RUN_STARTUP;
struct winsize win;
int disallocFlag=0;
struct node {char *name; int flag;};

struct node  tableMod[]= {
   {"shift",    1<<KG_SHIFT},
   {"anyAlt",   1<<KG_ALT | 1<<KG_ALTGR},
   {"leftAlt",  1<<KG_ALT},
   {"rightAlt", 1<<KG_ALTGR},
   {"control",  1<<KG_CTRL},
   {NULL,0}
};

   /* provide defaults */
int opt_mod     =  4;           /* control */
int opt_buf     =  0;           /* ask the kernel about it */
int opt_user    =  1;           /* allow user cfg files */



typedef struct DrawItem {
   short type;
   short pad;
   char *name;
   char *arg;   /* a cmd string */
   void *clientdata;  /* a (Draw *) for menus or whatever   */
   int (*fun)();
   struct DrawItem *next;
} DrawItem;

typedef struct Draw {
   short width;               /* length of longest item */
   short height;              /* the number of items */
   short uid;                 /* owner */
   short buttons;             /* which button */
   short fore,back,bord,head; /* colors */
   char *title;               /* name */
   time_t mtime;              /* timestamp of source file */
   DrawItem *menu;            /* the list of items */
   struct Draw *next;         /* chain */
} Draw;

typedef struct Posted {
   short x,y,X,Y;
   Draw *draw;
   unsigned char *dump;
   short colorcell;
   struct Posted *prev;
} Posted;

Draw *drawList=NULL;

/* support functions and vars */
int yyerror(char *s);
int yylex(void);

DrawItem *cfg_cat(DrawItem *, DrawItem *);
DrawItem *cfg_makeitem(int mode, char *msg, int(*fun)(), void *detail);


/*===================================================================*
 * This part of the source is devoted to reading the cfg file
 */

char cfgname[256];
FILE *cfgfile=NULL;
int cfglineno=0;
Draw *cfgcurrent, *cfgall;

Draw *cfg_alloc(void);

/* prototypes for predefined functions */

enum F_call {F_CREATE, F_POST, F_INVOKE, F_DONE};
int f_debug(int mode, DrawItem *self, int uid);
int f_bgcmd(int mode, DrawItem *self, int uid);
int f_fgcmd(int mode, DrawItem *self, int uid);
int f_jptty(int mode, DrawItem *self, int uid);
int f_mktty(int mode, DrawItem *self, int uid);
int f_menu(int mode, DrawItem *self, int uid);
int f_lock(int mode, DrawItem *self, int uid);
int f_load(int mode, DrawItem *self, int uid);
int f_free(int mode, DrawItem *self, int uid);
int f_time(int mode, DrawItem *self, int uid);
int f_pipe(int mode, DrawItem *self, int uid);

%} /* and this starts yacc definitions */

%union {
      int silly;
      char *string;
      Draw *draw;
      DrawItem *item;
      int (*fun)();
      }

%token <string> T_STRING
%token <silly> T_BACK T_FORE T_BORD T_HEAD
%token <silly> T_BRIGHT T_COLOR T_NAME

%token T_BUTTON
%token <fun> T_FUNC T_FUN2
%type <silly> bright button
%type <draw> menu file
%type <item> item items itemlist
%% /* begin grammar #########################################################*/

file: /* empty */               {$$=cfgall=NULL;}
      | file button menu      {$3->buttons=$2; $3->next=$1; $$=cfgall=$3;}
      ;

button: T_BUTTON '1' {$$=GPM_B_LEFT;}
         | T_BUTTON '2' {$$=GPM_B_MIDDLE;}
         | T_BUTTON '3' {$$=GPM_B_RIGHT;}
         ;

menu: '{'                       {$<draw>$=cfgcurrent=cfg_alloc();}
          configs itemlist '}'  {$$=$<draw>2; $$->menu=$4;}
            ;

configs: /* empty */ | configs cfgpair ;

cfgpair: T_NAME T_STRING         {cfgcurrent->title=$2;}
       | T_BACK T_COLOR          {cfgcurrent->back=$2;}
       | T_FORE bright T_COLOR   {cfgcurrent->fore=$3|$2;}
       | T_BORD bright T_COLOR   {cfgcurrent->bord=$3|$2;}
       | T_HEAD bright T_COLOR   {cfgcurrent->head=$3|$2;}
       ;

bright: /* empty */ {$$=0;} | T_BRIGHT {$$=8;} ;

itemlist: item items  {$$=cfg_cat($1,$2);}  ;

items: /* empty */ {$$=NULL;}
     | items item  {$$= $1 ? cfg_cat($1,$2) : $2;}
     ; 

item: T_STRING T_FUNC             {$$=cfg_makeitem('F',$1,$2, NULL);}
      | T_STRING T_FUN2 T_STRING    {$$=cfg_makeitem('2',$1,$2, $3);}
      | T_STRING menu               {$$=cfg_makeitem('M',$1,NULL,$2);}
      ;

%% /* end grammar ###########################################################*/

int yyerror(char *s)
{
   fprintf(stderr,"%s:%s(%i): %s\n",prgname,cfgname,cfglineno,s);
   return 1;
}

int yywrap()
{
   return 1;
}

struct tokenName {
   char *name;
   int token;
   int value;
   };
struct tokenName tokenList[] = {
   {"foreground",T_FORE,0},
   {"background",T_BACK,0},
   {"border",    T_BORD,0},
   {"head",      T_HEAD,0},
   {"name",      T_NAME,0},
   {"button",    T_BUTTON,0},
   {"black",     T_COLOR,0},
   {"blue",      T_COLOR,1},
   {"green",     T_COLOR,2},
   {"cyan",      T_COLOR,3},
   {"red",       T_COLOR,4},
   {"magenta",   T_COLOR,5},
   {"yellow",    T_COLOR,6},
   {"white",     T_COLOR,7},
   {"bright",    T_BRIGHT,0},
   {NULL,0,0}
   };

struct funcName {
   char *name;
   int token;
   int (*fun)();
   };
struct funcName funcList[] = {
   {"f.debug",T_FUNC,f_debug},
   {"f.fgcmd",T_FUN2,f_fgcmd},
   {"f.bgcmd",T_FUN2,f_bgcmd},
   {"f.jptty",T_FUN2,f_jptty},
   {"f.mktty",T_FUNC,f_mktty},
   {"f.menu",T_FUNC,f_menu},
   {"f.lock",T_FUN2,f_lock}, /* "lock one", "lock all" */
   {"f.load",T_FUNC,f_load},
   {"f.free",T_FUNC,f_free},
   {"f.time",T_FUNC,f_time},
   {"f.pipe",T_FUN2,f_pipe},
   {"f.nop",T_FUNC,NULL},
   {NULL,0,NULL}
};

/*---------------------------------------------------------------------*/
int yylex(void)
{
   int c,i;
   char s[80];
   struct tokenName *tn;
   struct funcName *fn;

   while(1) {
      i=0;
      switch(c=getc(cfgfile)) {
         case EOF: fclose(cfgfile); return 0;
         case '\"':
            do {
               s[i]=getc(cfgfile);
               if ((s[i])=='\n') {
                  yyerror("unterminated string");
                  cfglineno++;
               }
               if (s[i]=='\\') s[i]=getc(cfgfile);
            } /* get '"' as '\"' */ while (s[i++]!='\"' && s[i-2] !='\\') ;
            s[i-1]=0;
            yylval.string=(char *)strdup(s);
            return T_STRING;

         case '#': while ( (c=getc(cfgfile)!='\n') && c!=EOF) ;
         case '\n': cfglineno++;
         case ' ': /* fall through */
         case '\t': continue;
         default: if (!isalpha(c)) return(c);
      }
      /* get a single word and convert it */
      do {
         s[i++]=c;
      } while (isalnum(c=getc(cfgfile)) || c=='.');
      ungetc(c,cfgfile);
      s[i]=0;
      for (tn=tokenList; tn->name; tn++)
         if (tn->name[0]==s[0] && !strcmp(tn->name,s)) {
            yylval.silly=tn->value; 
            return tn->token;
         }
      for (fn=funcList; fn->name; fn++)
         if (fn->name[0]==s[0] && !strcmp(fn->name,s)) {
            yylval.fun=fn->fun; 
            return fn->token;
         }
      yylval.string=(char *)strdup(s); return T_STRING;
   }
} 

/*---------------------------------------------------------------------*/
void cfg_free(Draw *what)
{
   Draw *ptr;
   DrawItem *item;

   for (ptr=what; ptr; ptr=ptr->next) {
      if (ptr->title) free(ptr->title);
      for (item=ptr->menu; item; item=item->next) {
         if (item->name) free(item->name);
         if (item->arg) free(item->arg);
         if (item->type=='M' && item->clientdata) {
            ((Draw *)(item->clientdata))->next=NULL; /* redundant */
            cfg_free(item->clientdata);
         }
         if (item->clientdata) free(item->clientdata);
      }
   }
}

/*---------------------------------------------------------------------*/
/* malloc an empty Draw */
Draw *cfg_alloc(void)
{
   Draw *new=calloc(1,sizeof(Draw));

   if (!new) return NULL;
   new->back=DEFAULT_BACK;
   new->fore=DEFAULT_FORE;
   new->bord=DEFAULT_BORD;
   new->head=DEFAULT_HEAD;

   return new;
}

/*---------------------------------------------------------------------*/
/* malloc an empty DrawItem and fill it */
DrawItem *cfg_makeitem(int mode, char *msg, int(*fun)(), void *detail)
{
   DrawItem *new=calloc(1,sizeof(DrawItem));

   if (!new) return NULL;

   new->name=(char *)strdup(msg);
   new->type=mode;
   switch(mode) {
      case '2': /* a function with one arg */
         new->arg=(char *)strdup(detail);
         /* fall through */

      case 'F': /* a function without args */
         new->fun=fun;
         if (fun) fun(F_CREATE,new);
         break;

      case 'M':
         new->clientdata=detail;
         new->fun=f_menu;
         break;

      default: fprintf(stderr,"%s: unknown item type (can't happen)\n",prgname);
   }

   return new;
}

/*---------------------------------------------------------------------*/
/* concatenate two item lists */
DrawItem *cfg_cat(DrawItem *d1, DrawItem *d2)
{
   DrawItem *tmp;

   for (tmp=d1; tmp->next; tmp=tmp->next) ;
   tmp->next=d2;
   return d1;
}

/*====================================================================*/
void f__fix(struct passwd *pass)
{
   setgid(pass->pw_gid);
   initgroups(pass->pw_name, pass->pw_gid);
   setuid(pass->pw_uid);
   setenv("HOME",    pass->pw_dir, 1);
   setenv("LOGNAME", pass->pw_name,1);
   setenv("USER",    pass->pw_name,1);
}

/*---------------------------------------------------------------------*/
static int f_debug_one(FILE *f, Draw *draw)
{
   DrawItem *ip;
   static int tc=0;
   int i;

#define LINE(args) for(i=0;i<tc;i++) putc('\t',f); fprintf args

   LINE((f,"BUTT %i - %ix%i\n",draw->buttons,draw->width,draw->height));
   LINE((f,"UID %i\n",draw->uid));
   LINE((f,"fore %i - back %i\n",draw->fore,draw->back));
   LINE((f,"bord %i - head %i\n",draw->bord,draw->head));
   LINE((f,"---> \"%s\" %li\n",draw->title,(long)(draw->mtime)));
   for (ip=draw->menu; ip; ip=ip->next) {
      LINE((f,"    %i \"%s\" (%p)\n",ip->type,ip->name,ip->fun));
      if (ip->fun == f_menu) {
         tc++; f_debug_one(f,(Draw *)ip->clientdata); tc--;
      }
   }
#undef LINE
   return 0;
}

int f_debug(int mode, DrawItem *self, int uid)
{
#if 0 /* Disabled on account of security concerns; the way 
       * "/tmp/root-debug" is used is gratuitously
       * open to symlink abuse */

   FILE *f;
   Draw *dp;

   switch (mode) {
      case F_POST:
         if (!(f=fopen("/tmp/root-debug","a"))) return 1;
         for(dp=drawList; dp; dp=dp->next)
	         f_debug_one(f,dp);
         fprintf(f,"\n\n");
         fclose(f);

      case F_CREATE:
      case F_INVOKE:
         break;
      }
#endif /* 0 */
   return 0;
}


/*---------------------------------------------------------------------*/
int f_fgcmd(int mode, DrawItem *self, int uid)
{
   switch (mode) {
      case F_CREATE:
      case F_POST: break;
      case F_INVOKE: ; /* MISS */
   }
   return 0;
}

/*---------------------------------------------------------------------*/
int f_bgcmd(int mode, DrawItem *self, int uid)
{
   int i;
   struct passwd *pass;

   switch (mode) {
      case F_CREATE:
      case F_POST: break;
      case F_INVOKE:
         switch(fork()) {
	         case -1:
               gpm_report(GPM_PR_ERR, "fork(): %s", strerror(errno));
               return 1;
	         case 0:
	            pass=getpwuid(uid);
	            if (!pass) exit(1);
	            f__fix(pass); /* setgid(), setuid(), setenv(), ... */
	            close(0); close(1); close(2);
	            open("/dev/null",O_RDONLY); /* stdin  */
	            open(consolename,O_WRONLY); /* stdout */
	            dup(1);                     /* stderr */  
	            for (i=3;i<OPEN_MAX; i++) close(i);
	            execl("/bin/sh","sh","-c",self->arg,(char *)NULL);
	            exit(1); /* shouldn't happen */
	         default: return 0;

	      }
   }
   return 0;
}
/*---------------------------------------------------------------------*/
int f_jptty(int mode, DrawItem *self, int uid)
{
   int i,fd;

   switch (mode) {
      case F_CREATE:
      case F_POST: break;
      case F_INVOKE:
         i=atoi(self->arg);
         fd=open(consolename,O_RDWR);
         if (fd<0) {
            gpm_report(GPM_PR_ERR, "%s: %s",consolename, strerror(errno));
            return 1;
         } /*if*/
         if (ioctl(fd, VT_ACTIVATE, i)<0) {
            gpm_report(GPM_PR_ERR, "%s: %s", consolename,strerror(errno));
            return 1;
         } /*if*/
         if (ioctl(fd, VT_WAITACTIVE, i)<0) {
            gpm_report(GPM_PR_ERR, "%s: %s", consolename,strerror(errno));
            return 1;
         }
      default: return 0;
   }
   return 0; /* silly gcc -Wall */
}

/*---------------------------------------------------------------------*/
/* This array registers spawned consoles */
static int consolepids[1+MAX_NR_USER_CONSOLES];

int f_mktty(int mode, DrawItem *self, int uid)
{
   int fd, pid;
   int vc;
   char name[10];
   switch (mode) {
      case F_CREATE: self->arg=malloc(8);
      case F_POST: break;
      case F_INVOKE:
         fd=open(consolename,O_RDWR);
         if (fd<0) {
            gpm_report(GPM_PR_ERR,"%s: %s",consolename, strerror(errno));
            return 1;
         } /*if*/
         if (ioctl(fd, VT_OPENQRY, &vc)<0) {
            gpm_report(GPM_PR_ERR, "%s: %s",consolename, strerror(errno));
            return 1;
         } /*if*/
         switch(pid=fork()) {
	         case -1:
               gpm_report(GPM_PR_ERR, "fork(): %s", strerror(errno));
               return 1;
	         case 0: /* child: exec getty */
	            sprintf(name,"tty%i",vc);
	            execl("/sbin/mingetty","mingetty",name,(char *)NULL);
	            exit(1); /* shouldn't happen */
            default: /* father: jump to the tty */
               gpm_report(GPM_PR_INFO,"Registering child %i on console %i"
                                                                      ,pid,vc);
	            consolepids[vc]=pid;
	            sprintf(self->arg,"%i",vc);
	            return f_jptty(mode,self,uid);
	      }
      default: return 0;
   }
   return 0;
}

/*---------------------------------------------------------------------*/
int f_menu(int mode, DrawItem *self, int uid)
{
   return 0; /* just a placeholder, recursion is performed in main() */  
}

/*---------------------------------------------------------------------*/
int f_lock(int mode, DrawItem *self, int uid)
{
#if 0 /* some kind of interesting ...: if never */
   int all;
   static DrawItem msg = {
      0,
      10,
      "Enter your password to unlock",
      NULL, NULL, NULL, NULL
   };
   static Draw


   switch (mode) {
      case F_CREATE: /* either "one" or anything else */
         if (strcmp(self->arg,"one")) self->arg[0]='a';
      case F_POST: break;
      case F_INVOKE: /* the biggest of all... */
   }

#endif
   return 0;
}

/*---------------------------------------------------------------------*/
int f_load(int mode, DrawItem *self, int uid)
{
   FILE *f;
   double l1,l2,l3;

   l1=l2=l3=0.0;

   switch (mode) {
      case F_CREATE: /* modify name, just to fake its length */
         self->clientdata=malloc(strlen(self->name)+20);
         self->name=realloc(self->name,strlen(self->name)+20);
         strcpy(self->clientdata,self->name);
         strcat(self->clientdata," %5.2f %5.2f %5.2f");
         sprintf(self->name,self->clientdata,l1,l2,l3);
         break;

      case F_POST:
         if (!(f=fopen("/proc/loadavg","r"))) return 1;
         fscanf(f,"%lf %lf %lf",&l1,&l2,&l3);
         sprintf(self->name,self->clientdata,l1,l2,l3);
         fclose(f);

      case F_INVOKE: break;
   }
   return 0;
}

/*---------------------------------------------------------------------*/
int f_free(int mode, DrawItem *self, int uid)
{
   FILE *f;
   long l1,l2;
   char s[80];

   l1=l2=0;
   switch (mode) {
      case F_CREATE: /* modify name, just to fake its length */
         self->clientdata=malloc(strlen(self->name)+30);
         self->name=realloc(self->name,strlen(self->name)+30);
         strcpy(self->clientdata,self->name);
         strcat(self->clientdata," %5.2fM mem + %5.2fM swap");
         sprintf(self->name,self->clientdata,(double)l1,(double)l2);
         break;

      case F_POST:
         if (!(f=fopen("/proc/meminfo","r"))) return 1;
         fgets(s,80,f);
         fgets(s,80,f); sscanf(s,"%*s %*s %*s %li",&l1);
         fgets(s,80,f); sscanf(s,"%*s %*s %*s %li",&l2);
         sprintf(self->name,self->clientdata,
	      (double)l1/1024/1024,(double)l2/1024/1024);
         fclose(f);

      case F_INVOKE: break;
   }
   return 0;
}

/*---------------------------------------------------------------------*/
int f_time(int mode, DrawItem *self, int uid) {
   char s[128];
   struct tm *broken;
   time_t t;

   time(&t); broken=localtime(&t);
   switch (mode) {
      case F_CREATE: /* modify name, just to fake its length */
         self->clientdata=self->name;
         strftime(s,110,self->clientdata,broken);
         strcat(s,"1234567890"); /* names can change length */       
         self->name=(char *)strdup(s);
         /* rewrite the right string */
         strftime(self->name,110,self->clientdata,broken);
         break;

      case F_POST: strftime(self->name,120,self->clientdata,broken);
      case F_INVOKE: break;
   }
   return 0;
}

/*---------------------------------------------------------------------*/
int f_pipe(int mode, DrawItem *self, int uid)
{
   return 0;
}

/*====================================================================*/
int fixone(Draw *ptr, int uid)
{
   int hei,wid;
   DrawItem *item;

   ptr->uid=uid;
   hei=0; wid= ptr->title? strlen(ptr->title)+2 : 0;

   /* calculate width and height */
   for (item=ptr->menu; item; item=item->next) {
      hei++;
      wid= wid > strlen(item->name) ? wid : strlen(item->name);
   }
   ptr->height=hei+2;
   ptr->width=wid+2;

   /* fix paddings and recurse */
   for (item=ptr->menu; item; item=item->next) {
      item->pad=(ptr->width-strlen(item->name ? item->name : ""))/2;
      if (item->fun==f_menu) fixone((Draw *)item->clientdata,uid);
   }
   return 0;
}


/* read menus from a file, and return a list or NULL */
Draw *cfg_read(int uid)
{
   Draw *ptr;

   if (!(cfgfile=fopen(cfgname,"r"))) {
         gpm_report(GPM_PR_ERR, "%s: %s", cfgname, strerror(errno));
         return NULL;
   }
   gpm_report(GPM_PR_INFO,"Reading file %s",cfgname);
   cfglineno=1;
   if (yyparse()) {
         cfg_free(cfgall);
         cfgall=NULL;
         return NULL;
   }

   /* handle recursion */
   for (ptr=cfgall; ptr; ptr=ptr->next) {
      fixone(ptr,uid);
   }

   return cfgall;
}


/*---------------------------------------------------------------------*/
/* the return value tells whether it has been newly loaded or not */
int getdraw(int uid, int buttons, time_t mtime1, time_t mtime2)
{
   struct passwd *pass;
   struct stat buf;
   Draw *new, *np, *op, *pp;
   int retval=0;
   time_t mtime;

   gpm_report(GPM_PR_DEBUG,"getdraw: %i %i %li %li",uid,buttons,mtime1,mtime2);
   pass=getpwuid(uid);

   /* deny personal cfg to root for security reasons */
   if (pass==NULL || !uid || !opt_user) {
      mtime=mtime2; uid=-1;
      strcpy(cfgname,SYSTEM_CFG);
   } else {
      mtime=mtime1;
      strcpy(cfgname,pass->pw_dir);
      strcat(cfgname,"/" USER_CFG);
   }

   if (stat(cfgname,&buf)==-1) {
      gpm_report(GPM_PR_DEBUG,"stat (%s) failed",cfgname);
      /* try the system wide */
      mtime=mtime2; uid = -1;
      strcpy(cfgname,SYSTEM_CFG);
      if (stat(cfgname,&buf)==-1) {
         gpm_report(GPM_PR_ERR,"stat (%s) failed",cfgname);
         return 0;
      }
   }

   if (buf.st_mtime <= mtime) return 0;
  
   /* else, read the new drawing tree */
   new=cfg_read(uid);
   if (!new) return 0;

   /* scan old data to remove duplicates */
   for (np=pp=new; np; pp=np, np=np->next) {
      np->mtime=buf.st_mtime;
      if (np->buttons==buttons) retval++;
      for (op=drawList; op; op=op->next)
         if (op->uid==np->uid && op->buttons==np->buttons)
            op->buttons=0; /* mark for deletion */
      }

   /* chain in */
   pp->next=drawList; drawList=new;

   /* actually remove fake entries */
   for (np=drawList; np; pp=np, np=np->next)
      if (!np->buttons) {
         pp->next=np->next;
         np->next=NULL;
         cfg_free(np);
         np=pp;
      }
   return retval; /* found or not */
}


/*---------------------------------------------------------------------*/
Draw *retrievedraw(int uid, int buttons)
{
   Draw *drawPtr, *genericPtr=NULL;

   /* retrieve a drawing by scanning the list */
   do {
      for (drawPtr=drawList; drawPtr; drawPtr=drawPtr->next) {
         if (drawPtr->uid==uid && drawPtr->buttons==buttons) break;
         if (drawPtr->uid==-1 && drawPtr->buttons==buttons) genericPtr=drawPtr;
      }
   } while (getdraw(uid,buttons,
		 drawPtr ? drawPtr->mtime : 0,
		 genericPtr ? genericPtr->mtime :0));


   return drawPtr ? drawPtr : genericPtr;
}


/*=====================================================================*/
int usage(void)
{
   printf( GPM_MESS_VERSION "\n"
         "Usage: %s [options]\n",prgname);
   printf("  Valid options are\n"
         "    -m <number-or-name>   modifier to use\n"
         "    -u                    inhibit user configuration files\n"
         "    -D                    don't auto-background and run as daemon\n"
         "    -V <verbosity-delta>  increase amount of logged messages\n"
         );

   return 1;
}

/*------------*/
int getmask(char *arg, struct node *table)
{
   int last=0, value=0;
   char *cur;
   struct node *n;

   if (isdigit(arg[0])) return atoi(arg);

   while (1) {
      while (*arg && !isalnum(*arg)) arg++; /* skip delimiters */
      cur=arg;
      while(isalnum(*cur)) cur++; /* scan the word */
      if (!*cur) last++;
      *cur=0;

      for (n=table;n->name;n++)
         if (!strcmp(n->name,arg)) {
            value |= n->flag;
            break;
         }
         if(!n->name) fprintf(stderr,"%s: Incorrect flag \"%s\"\n",prgname,arg);
         if (last) break;
         cur++; arg=cur;
      }

   return value;
}

/*------------*/
int cmdline(int argc, char **argv)
{
   int opt;
  
   run_status = GPM_RUN_STARTUP;
   while ((opt = getopt(argc, argv,"m:uDV::")) != -1) {
         switch (opt) {
            case 'm':  opt_mod=getmask(optarg, tableMod); break;
            case 'u':  opt_user=0; break;
            case 'D':  run_status = GPM_RUN_DEBUG; break;
            case 'V':
               /*gpm_debug_level += (0==optarg ? 1 : strtol(optarg,0,0)); */
               break;
            default:   return 1;
         }

   }
   return 0;
}



/*------------*
 * This buffer is passed to set_selection, and the only meaningful value
 * is the last one, which is the mode: 4 means "clear_selection".
 * however, the byte just before the 1th short must be 2 which denotes
 * the selection-related stuff in ioctl(TIOCLINUX).
 */

static unsigned short clear_sel_args[6]={0, 0,0, 0,0, 4};
static unsigned char *clear_sel_arg= (unsigned char *)clear_sel_args+1;

/*------------*/
static inline void scr_dump(int fd, FILE *f, unsigned char *buffer, int vc)
{
   int dumpfd;
   char dumpname[20];

   sprintf(dumpname,"/dev/vcsa%i",vc);
   dumpfd=open(dumpname,O_RDONLY);
   if (dumpfd<0) {
      gpm_report(GPM_PR_ERR,"%s: %s", dumpname, strerror(errno));
      return;
   } /*if*/
   clear_sel_arg[0]=2;  /* clear_selection */
   ioctl(fd,TIOCLINUX,clear_sel_arg);
   read(dumpfd,buffer,4);
   read(dumpfd,buffer+4,2*buffer[0]*buffer[1]);
   close(dumpfd);
}

/*------------*/
static inline void scr_restore(int fd, FILE *f, unsigned char *buffer, int vc)
{
   int x,y, dumpfd;
   char dumpname[20];

   x=buffer[2]; y=buffer[3];
   
   /* WILL NOT WORK WITH DEVFS! FIXME! */
   sprintf(dumpname,"/dev/vcsa%i",vc);
   dumpfd=open(dumpname,O_WRONLY);
   if (dumpfd<0) {
      gpm_report(GPM_PR_ERR,"%s: %s", dumpname, strerror(errno));
      return;
   } /*if*/
   clear_sel_arg[0]=2;  /* clear_selection */
   ioctl(fd,TIOCLINUX,clear_sel_arg);
   write(dumpfd,buffer,4+2*buffer[0]*buffer[1]);
   close(dumpfd);
}

/*===================================================================*/
/* post and unpost menus from the screen */
static int postcount;
static Posted *activemenu;

#if __BYTE_ORDER == __BIG_ENDIAN
#define bigendian 1
#else
#define bigendian 0
#endif

Posted *postmenu(int fd, FILE *f, Draw *draw, int x, int y, int console)
{
   Posted *new;
   DrawItem *item;
   unsigned char *dump;
   unsigned char *curr, *curr2;
   int i;
   short lines,columns;

   new=calloc(1,sizeof(Posted));
   if (!new) return NULL;
   new->draw=draw;
   new->dump=dump=malloc(opt_buf);
   scr_dump(fd,f,dump,console);
   lines=dump[0]; columns=dump[1];
   i=(columns*dump[3]+dump[2])*2+1; /* where to get it */
   if (i<0) i=1;
   new->colorcell=dump[4+i-bigendian];
   gpm_report(GPM_PR_DEBUG,"Colorcell=%02x (at %i,%i = %i)",
                new->colorcell,dump[2],dump[3],i-bigendian);

   /* place the box relative to the mouse */
   if (!postcount) x -= draw->width/2; else x+=2;
   y++;

   /* fit inside the screen */
   if (x<1) x=1;
   if (x+draw->width >= columns) x=columns-1-draw->width;
   if (y+draw->height > lines+1) y=lines+1-draw->height;
   new->x=x; new->X=x+draw->width-1;
   new->y=y; new->Y=y+draw->height-1;

   /* these definitions are dirty hacks, but they help in writing to the screen */
#if __BYTE_ORDER == __BIG_ENDIAN
#define PUTC(c,f,b)   (*(curr++)=((b)<<4)+(f),*(curr++)=(c))
#else
#define PUTC(c,f,b)   (*(curr++)=(c),*(curr++)=((b)<<4)+(f))
#endif
#define PUTS(s,f,b)   for(curr2=s;*curr2;PUTC(*(curr2++),f,b))
#define GOTO(x,y)     (curr=dump+4+2*((y)*columns+(x)))

   x--; y--; /* /dev/vcs is zero based */
   ioctl(fd,TCXONC,TCOOFF); /* inhibit further prints */
   dump=malloc(opt_buf);
   memcpy(dump,new->dump,opt_buf); /* dup the buffer */
   /* top border */
   GOTO(x,y);
   PUTC(ULCORNER,draw->bord,draw->back);
   for (i=0; i<draw->width; i++) PUTC(HORLINE,draw->bord,draw->back);
   PUTC(URCORNER,draw->bord,draw->back);
   if (draw->title) {
         GOTO(x+(draw->width-strlen(draw->title))/2,y);
         PUTC(' ',draw->head,draw->back);
         PUTS(draw->title,draw->head,draw->back);
         PUTC(' ',draw->head,draw->back);
   }
   /* sides and items */
   for (item=draw->menu; y++, item; item=item->next) {
         if (item->fun) (*(item->fun))(F_POST,item);
         GOTO(x,y); PUTC(VERLINE,draw->bord,draw->back);
         for (i=0;i<item->pad;i++) PUTC(' ',draw->fore,draw->back);
         PUTS(item->name,draw->fore,draw->back); i+=strlen(item->name);
         while (i++<draw->width) PUTC(' ',draw->fore,draw->back);
         PUTC(VERLINE,draw->bord,draw->back);
   }
   /* bottom border */
   GOTO(x,y);
   PUTC(LLCORNER,draw->bord,draw->back);
   for (i=0; i<draw->width; i++) PUTC(HORLINE,draw->bord,draw->back);
   PUTC(LRCORNER,draw->bord,draw->back);

   scr_restore(fd,f,dump,console);
   free(dump);

#undef PUTC
#undef PUTS
#undef GOTO

   new->prev=activemenu;
   activemenu=new;
   postcount++;
   return new;
}

Posted *unpostmenu(int fd, FILE *f, Posted *which, int vc)
{
   Posted *prev=which->prev;

   scr_restore(fd,f,which->dump, vc);
   ioctl(fd,TCXONC,TCOON); /* activate the console */  
   free(which->dump);
   free(which);
   activemenu=prev;
   postcount--;
   return prev;
}


void reap_children(int signo)
{
   int i, pid;
   pid=wait(&i);
   gpm_report(GPM_PR_INFO,"pid %i exited %i",pid,i);

   if (disallocFlag)
      gpm_report(GPM_PR_INFO,"Warning, overriding logout from %i",disallocFlag);
   for (i=1;i<=MAX_NR_USER_CONSOLES; i++)
      if (consolepids[i]==pid) {
         disallocFlag=i;
         consolepids[i]=0;
         gpm_report(GPM_PR_INFO,"Registering disallocation of console %i",i);
         break;
      }
}


void get_winsize(void)
{
   int fd;

   if ((fd=open(consolename,O_RDONLY))<0) {
         fprintf(stderr,"%s: ",prgname); perror(consolename);
         exit(1);
   }
   ioctl(fd, TIOCGWINSZ, &win);
   opt_buf=win.ws_col*win.ws_row;
   close(fd);

   opt_buf +=4; /* 2:size, 1:terminator, 1:alignment */
   opt_buf*=2; /* the new scrdump and /dev/vcsa returns color info as well */
}


/*===================================================================*/
static int do_resize=0;
#if defined(__GLIBC__)
__sighandler_t winchHandler(int errno);
#else /* __GLIBC__ */
void winchHandler(int errno);
#endif /* __GLIBC__ */

int main(int argc, char **argv)
{
   Gpm_Connect conn;
   Gpm_Event ev;
   int vc, fd=-1 ,uid=-1;
   FILE *f=NULL;
   struct stat stbuf;
   Draw *draw=NULL;
   DrawItem *item;
   char s[80];
   int posty = 0, postx, postX;
   struct sigaction childaction;
   int evflag;
   int recursenow=0; /* not on first iteration */

   prgname=argv[0];
   consolename = Gpm_get_console();
   setuid(0); /* if we're setuid, force it */

   if (getuid()) {
         fprintf(stderr,"%s: Must be root\n", prgname);
         exit(1);
   }

   /*
   * Now, first of all we need to check that /dev/vcs is there.
   * But only if the kernel is new enough. vcs appeared in 1.1.82.
   * If an actual open fails, a message on syslog will be issued.
   */
   {
      struct utsname linux_info;
      int v1,v2,v3;
      struct stat sbuf;

      if (uname(&linux_info)) {
         fprintf(stderr,"%s: uname(): %s\n",prgname,strerror(errno));
         exit(1);
      }
      sscanf(linux_info.release,"%d.%d.%d",&v1,&v2,&v3);
      if (v1*1000000 + v2*1000 +v3 < 1001082) {
         fprintf(stderr,"%s: can't run with linux < 1.1.82\n",prgname);
         exit(1);
      }
      
      /* problems with devfs! FIXME! */
      if (stat("/dev/vcs0",&sbuf)<0 && stat("/dev/vcs",&sbuf)<0) {
         fprintf(stderr,"%s: /dev/vcs0: %s\n",prgname,strerror(errno));
         fprintf(stderr,"%s: do you have vcs devices? Refer to the manpage\n",
                prgname);
         exit(1);
      } else if (!S_ISCHR(sbuf.st_mode) ||
             VCS_MAJOR != major(sbuf.st_rdev) ||
             0 != minor(sbuf.st_rdev)) {
         fprintf(stderr,"Your /dev/vcs device looks funny\n");
         fprintf(stderr,"Refer to the manpage and possibly run the"
                        "create_vcs script in gpm source directory\n");
         exit(1);
      }
   }

   if (cmdline(argc,argv)) exit(usage());

   openlog(prgname, LOG_PID|LOG_CONS, run_status == GPM_RUN_DAEMON ?
                                                        LOG_DAEMON : LOG_USER);
   /* reap your zombies */
   childaction.sa_handler=reap_children;
#if defined(__GLIBC__)
   __sigemptyset(&childaction.sa_mask);
#else /* __GLIBC__ */
   childaction.sa_mask=0;
#endif /* __GLIBC__ */
   childaction.sa_flags=SA_INTERRUPT; /* need to break the select() call */
   sigaction(SIGCHLD,&childaction,NULL);

   /*....................................... Connect and get your buffer */

   conn.eventMask=GPM_DOWN;
   conn.defaultMask=GPM_MOVE; /* only ctrl-move gets the default */
   conn.maxMod=conn.minMod=opt_mod;

   gpm_zerobased=1;

   for (vc=4; vc-->0;)
      {
         extern int gpm_tried; /* liblow.c */
         gpm_tried=0; /* to enable retryings */
         if (Gpm_Open(&conn,-1)!=-1)
            break;
         if (vc)
            sleep(2);
      }
   if (!vc)
      {
         gpm_report(GPM_PR_OOPS,"can't open mouse connection");
      }

   conn.eventMask=~0; /* grab everything away form selection */
   conn.defaultMask=GPM_MOVE & GPM_HARD;
   conn.minMod=0;
   conn.maxMod=~0;

   chdir("/");


   get_winsize();

   /*....................................... Go to background */

   if (run_status != GPM_RUN_DEBUG) {
      switch(fork()) {
         case -1: gpm_report(GPM_PR_OOPS,"fork()");                  /* error  */
         case  0: run_status = GPM_RUN_DAEMON; break; /* child  */
         default: _exit(0);                           /* parent */
      }

      /* redirect stderr to /dev/console -- avoided now. 
         we should really cleans this more up! */
      fclose(stdin); fclose(stdout);
      if (!freopen(GPM_NULL_DEV,"w",stderr)) {
            gpm_report(GPM_PR_OOPS,"freopen(stderr)");
      }
      if (setsid()<0)
            gpm_report(GPM_PR_OOPS,"setsid()");

   } /*if*/

   /*....................................... Loop */

   while((evflag=Gpm_GetEvent(&ev))!=0)
      {
         if (do_resize) {get_winsize(); do_resize--;}

         if (disallocFlag)
            {
          struct utmp *uu;
          struct utmp u;
          char s[8];
          int i=0;
          
          gpm_report(GPM_PR_INFO,"Disallocating %i",disallocFlag);
          ioctl(fileno(stdin),VT_DISALLOCATE,&i); /* all of them */
          
          sprintf(s,"tty%i",disallocFlag);
          setutent();
          strncpy(u.ut_line, s, sizeof(u.ut_line));
          if ((uu = getutline(&u)) != 0)
            {
              uu->ut_type = DEAD_PROCESS ;
              pututline(uu);
            }
          disallocFlag=0;
            }

         if (evflag==-1) continue; /* no real event */

         /* get rid of spurious events */
         if (ev.type&GPM_MOVE) continue; 

         vc=ev.vc;
         gpm_report(GPM_PR_DEBUG,"%s: event on console %i at %i, %i",
                    prgname,ev.vc,ev.x,ev.y);

         if (!recursenow) /* don't open on recursion */
            {
          sprintf(s,"/dev/tty%i",ev.vc);
          if (stat(s,&stbuf)==-1) continue;
          uid = stbuf.st_uid;
          gpm_report(GPM_PR_DEBUG,"uid = %i",uid);

          draw=retrievedraw(uid,ev.buttons);
          if (!draw) continue;

          if (stat(s,&stbuf)==-1 || !(f=fopen(s,"r+"))) /* used to draw */
            {
              gpm_report(GPM_PR_ERR, "%s: %s", s, strerror(errno));
              continue;
            }
          
          if ((fd=open(s,O_RDWR))<0) /* will O_RDONLY be enough? */
            {
              gpm_report(GPM_PR_ERR, "%s: %s", s, strerror(errno));
              exit(1);
            }

          /* now change your connection information and manage the console */
          Gpm_Open(&conn,-1);
          uid=stbuf.st_uid;
            }

         /* the task now is drawing the box from user data */
         if (!draw)
            {
          /* itz Thu Jul  2 00:02:53 PDT 1998 this cannot happen, see
             continue statement above?!? */
          gpm_report(GPM_PR_ERR,"NULL menu ptr while drawing");
          continue;
            }
         postmenu(fd,f,draw,ev.x,ev.y,vc);

         while(Gpm_GetEvent(&ev)>0 && ev.vc==vc)
            {
          Gpm_FitEvent(&ev);
          if (ev.type&GPM_DOWN)
            break; /* we're done */
          Gpm_DrawPointer(ev.x,ev.y,fd);
            }
         gpm_report(GPM_PR_DEBUG,"%i - %i",posty,ev.y);

         /* ok, redraw, close and return waiting */
         gpm_report(GPM_PR_DEBUG,"Active is %p",activemenu->draw);
         posty=activemenu->y;
         postx=activemenu->x;
         postX=activemenu->X;

         recursenow=0; item=NULL; /* by default */
         posty=ev.y-posty;
         if (postx<=ev.x && ev.x<=postX) /* look for it */
            {
          for (item=draw->menu; posty-- && item; item=item->next)
            gpm_report(GPM_PR_DEBUG,"item %s (%p)",item->name, item->fun);
          if (item && item->fun && item->fun==f_menu)
            {
              recursenow++;
              draw=item->clientdata;
              continue;
            }
            }

         /* unpost them all */
         while (unpostmenu(fd,f,activemenu,vc))
            ;
         close(fd);
         fclose(f);
         Gpm_Close();
         recursenow=0; /* just in case... */

         /* invoke the item */
         if (item && item->fun)
            (*(item->fun))(F_INVOKE,item,uid);
      }

   /*....................................... Done */

   while (Gpm_Close()) ; /* close all the stack */ 
   exit(0);
}

/* developers chat:
 * author1 (possibly alessandro):
   "This is because Linus uses 4-wide tabstops, forcing me to use the same
   default to manage kernel sources"
  * ian zimmermann (alias itz) on Wed Jul  1 23:28:13 PDT 1998:
   "I don't mind what anybody's physical tab size is, but when I load it into
   the editor I don't want any wrapping lines."
  * nico schottelius (january 2002): 
   "Although Linux document /usr/src/linux/Documentation/CodingStyle is mostly
   correct, I agree with itz to avoid wrapping lines. Merging 4(alessandro)
   /2(itz) spaces makes 3 which is the current standard."
  */ 

/* Local Variables: */
/* tab-width:3      */
/* c-indent-level: 3 */
/* End:             */
