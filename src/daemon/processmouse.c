/*-------------------------------------------------------------------
 * call getMouseData to get hardware device data, call mouse device's fun() 
 * to retrieve the hardware independent event data, then optionally repeat
 * the data via repeat_fun() to the repeater device
 *-------------------------------------------------------------------*/
static inline int processMouse(int fd, Gpm_Event *event, Gpm_Type *type,
                int kd_mode)
{
   char *data;
   static int fine_dx, fine_dy;
   static int i, j, m;
   static Gpm_Event nEvent;
   static struct vt_stat stat;
   static struct timeval tv1={0,0}, tv2; /* tv1==0: first click is single */
   static struct timeval timeout={0,0};
   fd_set fdSet;
   static int newB=0, oldB=0, oldT=0; /* old buttons and Type to chain events */
   /* static int buttonlock, buttonlockflag; */

#define GET_TIME(tv) (gettimeofday(&tv, (struct timezone *)NULL))
#define DIF_TIME(t1,t2) ((t2.tv_sec -t1.tv_sec) *1000+ \
                          (t2.tv_usec-t1.tv_usec)/1000)


   oldT=event->type;

   if (eventFlag) {
      eventFlag=0;

      if (m_type->absolute) {     /* a pen or other absolute device */
         event->x=nEvent.x;
         event->y=nEvent.y;
      }
      event->dx=nEvent.dx;
      event->dy=nEvent.dy;
      event->buttons=nEvent.buttons;
   } else {
      event->dx=event->dy=0;
      event->wdx=event->wdy=0;
      nEvent.modifiers = 0; /* some mice set them */
      FD_ZERO(&fdSet); FD_SET(fd,&fdSet); i=0;

      do { /* cluster loop */
         if(((data=getMouseData(fd,m_type,kd_mode))==NULL)
            || ((*(m_type->fun))(&nEvent,data)==-1) ) {
            if (!i) return 0;
            else break;
         }

         event->modifiers = nEvent.modifiers; /* propagate modifiers */

         /* propagate buttons */
         nEvent.buttons = (opt_sequence[nEvent.buttons&7]&7) |
            (nEvent.buttons & ~7); /* change the order */
         oldB=newB; newB=nEvent.buttons;
         if (!i) event->buttons=nEvent.buttons;

         if (oldB!=newB) {
            eventFlag = (i!=0)*(which_mouse-mouse_table); /* 1 or 2 */
            break;
         }

         /* propagate movement */
         if (!(m_type->absolute)) { /* mouse */
            if (abs(nEvent.dx)+abs(nEvent.dy) > opt_delta)
               nEvent.dx*=opt_accel, nEvent.dy*=opt_accel;

            /* increment the reported dx,dy */
            event->dx+=nEvent.dx;
            event->dy+=nEvent.dy;
         } else { /* a pen */
            /* get dx,dy to check if there has been movement */
            event->dx = (nEvent.x) - (event->x);
            event->dy = (nEvent.y) - (event->y);
         }

         /* propagate wheel */
         event->wdx += nEvent.wdx;
         event->wdy += nEvent.wdy;

         select(fd+1,&fdSet,(fd_set *)NULL,(fd_set *)NULL,&timeout/* zero */);

      } while (i++ <opt_cluster && nEvent.buttons==oldB && FD_ISSET(fd,&fdSet));
     
   } /* if(eventFlag) */

/*....................................... update the button number */

   if ((event->buttons&GPM_B_MIDDLE) && !opt_three) opt_three++;

/*....................................... we're a repeater, aren't we? */

   if (kd_mode!=KD_TEXT) {
      if (fifofd != -1 && ! opt_rawrep) {
         if (m_type->absolute) { /* hof Wed Feb  3 21:43:28 MET 1999 */ 
            /* prepare the values from a absolute device for repeater mode */
            static struct timeval rept1,rept2;
            gettimeofday(&rept2, (struct timezone *)NULL);
            if (((rept2.tv_sec -rept1.tv_sec)
                  *1000+(rept2.tv_usec-rept1.tv_usec)/1000)>250) {
               event->dx=0;
               event->dy=0;
            }
            rept1=rept2;
              
            event->dy=event->dy*((win.ws_col/win.ws_row)+1);
            event->x=nEvent.x; 
            event->y=nEvent.y;
         }
         repeated_type->repeat_fun(event, fifofd); /* itz Jan 11 1999 */
      }
      return 0; /* no events nor information for clients */
   } /* first if of these three */

/*....................................... no, we arent a repeater, go on */

   /* use fine delta values now, if delta is the information */
   if (!(m_type)->absolute) {
      fine_dx+=event->dx;             fine_dy+=event->dy;
      event->dx=fine_dx/opt_scale;    event->dy=fine_dy/opt_scaley;
      fine_dx %= opt_scale;           fine_dy %= opt_scaley;
   }

   /* up and down, up and down, ... who does a do..while(0) loop ???
      and then makes a break into it... argh ! */

   if (!event->dx && !event->dy && (event->buttons==oldB))
      do { /* so to break */
         static long awaketime;
         /*
          * Ret information also if never happens, but enough time has elapsed.
          * Note: return 1 will segfault due to missing event->vc; FIXME!
          */
         if (time(NULL)<=awaketime) return 0;
         awaketime=time(NULL)+1;
         break;
      } while (0);

/*....................................... fill missing fields */

   event->x+=event->dx, event->y+=event->dy;
   statusB=event->buttons;

   i=open_console(O_RDONLY);
   /* modifiers */
   j = event->modifiers; /* save them */
   event->modifiers=6; /* code for the ioctl */
   if (ioctl(i,TIOCLINUX,&(event->modifiers))<0)
      gpm_report(GPM_PR_OOPS,GPM_MESS_GET_SHIFT_STATE);
   event->modifiers |= j; /* add mouse-specific bits */

   /* status */
   j = stat.v_active;
   if (ioctl(i,VT_GETSTATE,&stat)<0) gpm_report(GPM_PR_OOPS,GPM_MESS_GET_CONSOLE_STAT);

   /*
    * if we changed console, request the current console size,
    * as different consoles can be of different size
    */
   if (stat.v_active != j)
      get_console_size(event);
   close(i);

   event->vc = stat.v_active;

   if (oldB==event->buttons)
      event->type = (event->buttons ? GPM_DRAG : GPM_MOVE);
   else
      event->type = (event->buttons > oldB ? GPM_DOWN : GPM_UP);

   switch(event->type) {                /* now provide the cooked bits */
      case GPM_DOWN:
         GET_TIME(tv2);
         if (tv1.tv_sec && (DIF_TIME(tv1,tv2)<opt_time)) /* check first click */
            statusC++, statusC%=3; /* 0, 1 or 2 */
         else
            statusC=0;
         event->type|=(GPM_SINGLE<<statusC);
         break;

      case GPM_UP:
         GET_TIME(tv1);
         event->buttons^=oldB; /* for button-up, tell which one */
         event->type|= (oldT&GPM_MFLAG);
         event->type|=(GPM_SINGLE<<statusC);
         break;

      case GPM_DRAG:
         event->type |= GPM_MFLAG;
         event->type|=(GPM_SINGLE<<statusC);
         break;

      case GPM_MOVE:
         statusC=0;
      default:
         break;
   }
   event->clicks=statusC;
   
/* UGLY - FIXME! */
/* The current policy is to force the following behaviour:
 * - At buttons up, must fit inside the screen, though flags are set.
 * - At button down, allow going outside by one single step
 */


   /* selection used 1-based coordinates, so do I */

   /*
    * 1.05: only one margin is current. Y takes priority over X.
    * The i variable is how much margin is allowed. "m" is which one is there.
    */

   m = 0;
   i = ((event->type&(GPM_DRAG|GPM_UP))!=0); /* i is boolean */

   if (event->y>win.ws_row)  {event->y=win.ws_row+1-!i; i=0; m = GPM_BOT;}
   else if (event->y<=0)     {event->y=1-i;             i=0; m = GPM_TOP;}

   if (event->x>win.ws_col)  {event->x=win.ws_col+1-!i; if (!m) m = GPM_RGT;}
   else if (event->x<=0)     {event->x=1-i;             if (!m) m = GPM_LFT;}

   event->margin=m;

   gpm_report(GPM_PR_DEBUG,"M: %3i %3i (%3i %3i) - butt=%i vc=%i cl=%i",
                                       event->dx,event->dy,
                                       event->x,event->y,
                                       event->buttons, event->vc,
                                       event->clicks);

   /* update the global state */
   statusX=event->x; statusY=event->y;

   if (opt_special && event->type & GPM_DOWN) 
      return processSpecial(event);

   return 1;
}
