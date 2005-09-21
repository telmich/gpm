#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef HAVE_SYS_SYSMACROS_H
# include <sys/sysmacros.h>
#else
# define minor(dev) (((unsigned int) (dev)) >> 8)
# define major(dev) ((dev) & 0xff)
#endif

#ifdef HAVE_LINUX_MAJOR_H
# include <linux/major.h>
#else
# ifndef TTY_MAJOR
#  define TTY_MAJOR 4
# endif
# ifndef VCS_MAJOR
#  define VCS_MAJOR 7
# endif
#endif

#include "gpmInt.h"

/*
 * Arghhh.  check_devfs was soo brain dead.  We just need to determine
 * whether or not we are using devfs.  In that case we should do the
 * nice thing and return a boolean value.
 *
 * To do this we check if the /dev/.devfsd file exists.  If it does,
 * then we are most likely running devfs.  We should write another
 * function which returns the /dev/XXX portion of the appropriate
 * devices.  This is a start and a building block for such a function.
 */

int using_devfs( void ) {
    struct stat devfs_node;
  
    return (stat("/dev/.devfsd", (&devfs_node))
	    && S_ISCHR(devfs_node.st_mode));
}

#define LOOP_SET_SUCCESS_MAJOR(var,dev_major,stat) \
do {						\
    for(i=0; var##_name[i] != NULL; i++)	\
	if (stat(var##_name[i], &buf) == 0	\
	    && S_ISCHR(buf.st_mode)		\
	    && major(buf.st_rdev) == dev_major	\
	    && minor(buf.st_rdev) == 0)		\
                                                \
	    return var = var##_name[i];		\
} while(0)


/* We may not be able to open the silly thing!!! */

const char * get_virtualconsole0( void ) {
    static const char * const console_name[] = {
	_PATH_TTY "0",
	"/dev/tty0",
	"/dev/vc/0",
	NULL
    };
    static const char * console = NULL;
    struct stat buf;
    int i;

    if (console)
	return console;

    LOOP_SET_SUCCESS_MAJOR(console,TTY_MAJOR,lstat);

    /* If we failed as yet ... try again, this time with stat.
       Someone is bound to leave a symlink around for compatiblity. */

    LOOP_SET_SUCCESS_MAJOR(console,TTY_MAJOR,stat);

    oops("Unable to determine virtual console device");
}

const char * get_virtualconsole_base( void ) {
    static char * console_base = NULL;

    if (console_base == NULL) {
	console_base = strdup(get_virtualconsole0());
	console_base[ strlen(console_base)-1 ] = '\0';
    }

    return console_base;
}	    

const char * get_virtualscreen0( void ) {
    static const char * const screen_name[] = {
	"/dev/vcs",
	"/dev/vcs0",
	"/dev/vcc/0",
	NULL
    };
    static const char * screen = NULL;
    struct stat buf;
    int i;

    if (screen)
	return screen;

    LOOP_SET_SUCCESS_MAJOR(screen,VCS_MAJOR,lstat);

    /* try again ... maybe there are others ... at least we can assume
       some compatiblity from time to time... */

    LOOP_SET_SUCCESS_MAJOR(screen,VCS_MAJOR,stat);

    oops("Unable to determine virtual console screen device");
}

const char * get_virtualscreen_base( void ) {
    static char * screen_base = NULL;

    if (screen_base == NULL) {
	screen_base = strdup(get_virtualscreen0());
	if (using_devfs()
	    || screen_base[ strlen(screen_base)-1 ] == '0')
	    screen_base[ strlen(screen_base)-1 ] = '\0';
    }

    return screen_base;
}
/* Local Variables: */
/* tab-width:8      */
/* c-indent-level:4 */
/* End:             */
