
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 27 "/home/hamarti/libraries/gpm/gpm-1.20.6/src/prog/gpm-root.y"


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



/* Line 189 of yacc.c  */
#line 240 "y.tab.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     T_STRING = 258,
     T_BACK = 259,
     T_FORE = 260,
     T_BORD = 261,
     T_HEAD = 262,
     T_BRIGHT = 263,
     T_COLOR = 264,
     T_NAME = 265,
     T_BUTTON = 266,
     T_FUNC = 267,
     T_FUN2 = 268
   };
#endif
/* Tokens.  */
#define T_STRING 258
#define T_BACK 259
#define T_FORE 260
#define T_BORD 261
#define T_HEAD 262
#define T_BRIGHT 263
#define T_COLOR 264
#define T_NAME 265
#define T_BUTTON 266
#define T_FUNC 267
#define T_FUN2 268




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 193 "/home/hamarti/libraries/gpm/gpm-1.20.6/src/prog/gpm-root.y"

      int silly;
      char *string;
      Draw *draw;
      DrawItem *item;
      int (*fun)();
      


/* Line 214 of yacc.c  */
#line 312 "y.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 324 "y.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   29

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  19
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  11
/* YYNRULES -- Number of rules.  */
#define YYNRULES  23
/* YYNRULES -- Number of states.  */
#define YYNSTATES  37

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   268

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    14,
      15,    16,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    17,     2,    18,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint8 yyprhs[] =
{
       0,     0,     3,     4,     8,    11,    14,    17,    18,    24,
      25,    28,    31,    34,    38,    42,    46,    47,    49,    52,
      53,    56,    59,    63
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      20,     0,    -1,    -1,    20,    21,    22,    -1,    11,    14,
      -1,    11,    15,    -1,    11,    16,    -1,    -1,    17,    23,
      24,    27,    18,    -1,    -1,    24,    25,    -1,    10,     3,
      -1,     4,     9,    -1,     5,    26,     9,    -1,     6,    26,
       9,    -1,     7,    26,     9,    -1,    -1,     8,    -1,    29,
      28,    -1,    -1,    28,    29,    -1,     3,    12,    -1,     3,
      13,     3,    -1,     3,    22,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint8 yyrline[] =
{
       0,   212,   212,   213,   216,   217,   218,   221,   221,   225,
     225,   227,   228,   229,   230,   231,   234,   234,   236,   238,
     239,   242,   243,   244
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "T_STRING", "T_BACK", "T_FORE", "T_BORD",
  "T_HEAD", "T_BRIGHT", "T_COLOR", "T_NAME", "T_BUTTON", "T_FUNC",
  "T_FUN2", "'1'", "'2'", "'3'", "'{'", "'}'", "$accept", "file", "button",
  "menu", "@1", "configs", "cfgpair", "bright", "itemlist", "items",
  "item", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,    49,    50,    51,   123,   125
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    19,    20,    20,    21,    21,    21,    23,    22,    24,
      24,    25,    25,    25,    25,    25,    26,    26,    27,    28,
      28,    29,    29,    29
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     3,     2,     2,     2,     0,     5,     0,
       2,     2,     2,     3,     3,     3,     0,     1,     2,     0,
       2,     2,     3,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     0,     1,     0,     0,     4,     5,     6,     7,     3,
       9,     0,     0,     0,    16,    16,    16,     0,    10,     0,
      19,    21,     0,    23,    12,    17,     0,     0,     0,    11,
       8,    18,    22,    13,    14,    15,    20
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     1,     4,     9,    10,    11,    18,    26,    19,    31,
      20
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -10
static const yytype_int8 yypact[] =
{
     -10,     0,   -10,     1,    -5,   -10,   -10,   -10,   -10,   -10,
     -10,    -2,    -3,     4,    10,    10,    10,    16,   -10,     2,
     -10,   -10,    18,   -10,   -10,   -10,    13,    14,    15,   -10,
     -10,    22,   -10,   -10,   -10,   -10,   -10
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -10,   -10,   -10,    17,   -10,   -10,   -10,    -9,   -10,   -10,
      -4
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
       2,    12,    13,    14,    15,    16,    27,    28,    17,    21,
      22,     3,     8,    24,     8,     5,     6,     7,    25,    29,
      30,    32,    33,    34,    35,    12,     0,    36,     0,    23
};

static const yytype_int8 yycheck[] =
{
       0,     3,     4,     5,     6,     7,    15,    16,    10,    12,
      13,    11,    17,     9,    17,    14,    15,    16,     8,     3,
      18,     3,     9,     9,     9,     3,    -1,    31,    -1,    12
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    20,     0,    11,    21,    14,    15,    16,    17,    22,
      23,    24,     3,     4,     5,     6,     7,    10,    25,    27,
      29,    12,    13,    22,     9,     8,    26,    26,    26,     3,
      18,    28,     3,     9,     9,     9,    29
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{


    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

/* Line 1455 of yacc.c  */
#line 212 "/home/hamarti/libraries/gpm/gpm-1.20.6/src/prog/gpm-root.y"
    {(yyval.draw)=cfgall=NULL;}
    break;

  case 3:

/* Line 1455 of yacc.c  */
#line 213 "/home/hamarti/libraries/gpm/gpm-1.20.6/src/prog/gpm-root.y"
    {(yyvsp[(3) - (3)].draw)->buttons=(yyvsp[(2) - (3)].silly); (yyvsp[(3) - (3)].draw)->next=(yyvsp[(1) - (3)].draw); (yyval.draw)=cfgall=(yyvsp[(3) - (3)].draw);}
    break;

  case 4:

/* Line 1455 of yacc.c  */
#line 216 "/home/hamarti/libraries/gpm/gpm-1.20.6/src/prog/gpm-root.y"
    {(yyval.silly)=GPM_B_LEFT;}
    break;

  case 5:

/* Line 1455 of yacc.c  */
#line 217 "/home/hamarti/libraries/gpm/gpm-1.20.6/src/prog/gpm-root.y"
    {(yyval.silly)=GPM_B_MIDDLE;}
    break;

  case 6:

/* Line 1455 of yacc.c  */
#line 218 "/home/hamarti/libraries/gpm/gpm-1.20.6/src/prog/gpm-root.y"
    {(yyval.silly)=GPM_B_RIGHT;}
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 221 "/home/hamarti/libraries/gpm/gpm-1.20.6/src/prog/gpm-root.y"
    {(yyval.draw)=cfgcurrent=cfg_alloc();}
    break;

  case 8:

/* Line 1455 of yacc.c  */
#line 222 "/home/hamarti/libraries/gpm/gpm-1.20.6/src/prog/gpm-root.y"
    {(yyval.draw)=(yyvsp[(2) - (5)].draw); (yyval.draw)->menu=(yyvsp[(4) - (5)].item);}
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 227 "/home/hamarti/libraries/gpm/gpm-1.20.6/src/prog/gpm-root.y"
    {cfgcurrent->title=(yyvsp[(2) - (2)].string);}
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 228 "/home/hamarti/libraries/gpm/gpm-1.20.6/src/prog/gpm-root.y"
    {cfgcurrent->back=(yyvsp[(2) - (2)].silly);}
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 229 "/home/hamarti/libraries/gpm/gpm-1.20.6/src/prog/gpm-root.y"
    {cfgcurrent->fore=(yyvsp[(3) - (3)].silly)|(yyvsp[(2) - (3)].silly);}
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 230 "/home/hamarti/libraries/gpm/gpm-1.20.6/src/prog/gpm-root.y"
    {cfgcurrent->bord=(yyvsp[(3) - (3)].silly)|(yyvsp[(2) - (3)].silly);}
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 231 "/home/hamarti/libraries/gpm/gpm-1.20.6/src/prog/gpm-root.y"
    {cfgcurrent->head=(yyvsp[(3) - (3)].silly)|(yyvsp[(2) - (3)].silly);}
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 234 "/home/hamarti/libraries/gpm/gpm-1.20.6/src/prog/gpm-root.y"
    {(yyval.silly)=0;}
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 234 "/home/hamarti/libraries/gpm/gpm-1.20.6/src/prog/gpm-root.y"
    {(yyval.silly)=8;}
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 236 "/home/hamarti/libraries/gpm/gpm-1.20.6/src/prog/gpm-root.y"
    {(yyval.item)=cfg_cat((yyvsp[(1) - (2)].item),(yyvsp[(2) - (2)].item));}
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 238 "/home/hamarti/libraries/gpm/gpm-1.20.6/src/prog/gpm-root.y"
    {(yyval.item)=NULL;}
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 239 "/home/hamarti/libraries/gpm/gpm-1.20.6/src/prog/gpm-root.y"
    {(yyval.item)= (yyvsp[(1) - (2)].item) ? cfg_cat((yyvsp[(1) - (2)].item),(yyvsp[(2) - (2)].item)) : (yyvsp[(2) - (2)].item);}
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 242 "/home/hamarti/libraries/gpm/gpm-1.20.6/src/prog/gpm-root.y"
    {(yyval.item)=cfg_makeitem('F',(yyvsp[(1) - (2)].string),(yyvsp[(2) - (2)].fun), NULL);}
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 243 "/home/hamarti/libraries/gpm/gpm-1.20.6/src/prog/gpm-root.y"
    {(yyval.item)=cfg_makeitem('2',(yyvsp[(1) - (3)].string),(yyvsp[(2) - (3)].fun), (yyvsp[(3) - (3)].string));}
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 244 "/home/hamarti/libraries/gpm/gpm-1.20.6/src/prog/gpm-root.y"
    {(yyval.item)=cfg_makeitem('M',(yyvsp[(1) - (2)].string),NULL,(yyvsp[(2) - (2)].draw));}
    break;



/* Line 1455 of yacc.c  */
#line 1675 "y.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 247 "/home/hamarti/libraries/gpm/gpm-1.20.6/src/prog/gpm-root.y"
 /* end grammar ###########################################################*/

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
   if (setgid(pass->pw_gid) < 0 ||
       initgroups(pass->pw_name, pass->pw_gid) < 0 ||
       setuid(pass->pw_uid) < 0)
   exit(1);
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
		    int open_max = sysconf(_SC_OPEN_MAX);
		    if (open_max == -1) open_max = 1024;
	            for (i=3;i<open_max; i++) close(i);
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

