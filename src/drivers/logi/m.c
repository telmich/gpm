static int M_logi(Gpm_Event *state,  unsigned char *data) /* equal to mm */
{
   state->buttons= data[0] & 0x07;
   state->dx=      (data[0] & 0x10) ?   data[1] : - data[1];
   state->dy=      (data[0] & 0x08) ? - data[2] :   data[2];
   return 0;
}

