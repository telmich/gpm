static inline int processRequest(Gpm_Cinfo *ci, int vc) 
{
   int i;
   Gpm_Cinfo *cinfoPtr, *next;
   Gpm_Connect conn;
   static Gpm_Event event;
   static struct vt_stat stat;

   gpm_report(GPM_PR_INFO,GPM_MESS_CON_REQUEST, ci->fd, vc);
   if (vc>MAX_VC) return -1;

   /* itz 10-22-96 this shouldn't happen now */
   if (vc==-1) gpm_report(GPM_PR_OOPS,GPM_MESS_UNKNOWN_FD);

   i=get_data(&conn,ci->fd);

   if (!i) { /* no data */
      gpm_report(GPM_PR_INFO,GPM_MESS_CLOSE);
      close(ci->fd);
      FD_CLR(ci->fd,&connSet);
      FD_CLR(ci->fd,&readySet);
      if (cinfo[vc]->fd == ci->fd) { /* it was on top of the stack */
         cinfoPtr = cinfo[vc];
         cinfo[vc]=cinfo[vc]->next; /* pop the stack */
         free(cinfoPtr);
         return -1;
      }
      /* somewhere inside the stack, have to walk it */
      cinfoPtr = cinfo[vc];
      while (cinfoPtr && cinfoPtr->next) {
         if (cinfoPtr->next->fd == ci->fd) {
            next = cinfoPtr->next;
            cinfoPtr->next = next->next;
            free (next);
            break;
         }
         cinfoPtr = cinfoPtr->next;
      }
      return -1;
   } /* not data */
 
   if (i == -1) return -1; /* too few bytes */

   if (conn.pid!=0) {
      ci->data = conn;
      return 0;
   }

   /* Aha, request for information (so-called snapshot) */
   switch(conn.vc) {
      case GPM_REQ_SNAPSHOT:
         i=open_console(O_RDONLY);
         ioctl(i,VT_GETSTATE,&stat);
         event.modifiers=6; /* code for the ioctl */
         if (ioctl(i,TIOCLINUX,&(event.modifiers))<0)
         gpm_report(GPM_PR_OOPS,GPM_MESS_GET_SHIFT_STATE);
         close(i);
         event.vc = stat.v_active;
         event.x=statusX;   event.y=statusY;
         event.dx=maxx;     event.dy=maxy;
         event.buttons= statusB;
         event.clicks=statusC;
         /* fall through */
         /* missing break or do you want this ??? */

      case GPM_REQ_BUTTONS:
         event.type= (opt_three==1 ? 3 : 2); /* buttons */
         write(ci->fd,&event,sizeof(Gpm_Event));
         break;

      case GPM_REQ_NOPASTE:
         disable_paste(vc);
         break;
   }

   return 0;
}

/*-------------------------------------------------------------------*/
