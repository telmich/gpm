/* Thu Jan 28 20:54:47 MET 1999 hof@hof-berlin.de SummaSketch reportformat */
int R_summa(Gpm_Event *state, int fd)
{
   signed char buffer[5];
   static int x,y;

   if (realposx==-1) { /* real absolute device? */
      x=x+(state->dx*25); if (x<0) x=0; if (x>16383) x=16383;
      y=y+(state->dy*25); if (y<0) y=0; if (y>16383) y=16383;
   } else { /* yes */
      x=realposx; y=realposy;
   }

   buffer[0]=0x98+((state->buttons&GPM_B_LEFT)>0?1:0)+
                  ((state->buttons&GPM_B_MIDDLE)>0?2:0)+
                  ((state->buttons&GPM_B_RIGHT)>0?3:0);
   buffer[1]=x & 0x7f;  /* X0-6*/
   buffer[2]=(x >> 7) & 0x7f;  /* X7-12*/
   buffer[3]=y & 0x7f;  /* Y0-6*/
   buffer[4]=(y >> 7) & 0x7f;  /* Y7-12*/

   return write(fd,buffer,5);
}

