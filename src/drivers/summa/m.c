/* Summagraphics/Genius/Acecad tablet support absolute mode*/
/* Summagraphics MM-Series format*/
/* (Frank Holtz) hof@bigfoot.de Tue Feb 23 21:04:09 MET 1999 */
int SUMMA_BORDER=100;
int summamaxx,summamaxy;
char summaid=-1;
int M_summa(Gpm_Event *state, unsigned char *data)
{
   int x, y;

   x = ((data[2]<<7) | data[1])-SUMMA_BORDER;
   if (x<0) x=0;
   if (x>summamaxx) x=summamaxx;
   state->x = (x * win.ws_col / summamaxx);
   realposx = (x * 16383 / summamaxx);

   y = ((data[4]<<7) | data[3])-SUMMA_BORDER;
   if (y<0) y=0; if (y>summamaxy) y=summamaxy;
   state->y = 1 + y * (win.ws_row-1)/summamaxy;
   realposy = y * 16383 / summamaxy;

   state->buttons=
    !!(data[0]&1) * GPM_B_LEFT +
    !!(data[0]&2) * GPM_B_RIGHT +
    !!(data[0]&4) * GPM_B_MIDDLE;

   return 0;
}

