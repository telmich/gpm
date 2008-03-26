/* Calcomp UltraSlate tablet (John Anderson)
 * modeled after Wacom and NCR drivers (thanks, guys)
 * Note: I don't know how to get the tablet size from the tablet yet, so
 * these defines will have to do for now.
 */

#define CAL_LIMIT_BRK 255

#define CAL_X_MIN 0x40
#define CAL_X_MAX 0x1340
#define CAL_X_SIZE (CAL_X_MAX - CAL_X_MIN)

#define CAL_Y_MIN 0x40
#define CAL_Y_MAX 0xF40
#define CAL_Y_SIZE (CAL_Y_MAX - CAL_Y_MIN)

static int M_calus_rel(Gpm_Event *state, unsigned char *data)
{
   static int ox=-1, oy;
   int x, y;

   x = ((data[1] & 0x3F)<<7) | (data[2] & 0x7F);
   y = ((data[4] & 0x1F)<<7) | (data[5] & 0x7F);

   if (ox==-1 || abs(x-ox)>CAL_LIMIT_BRK || abs(y-oy)>CAL_LIMIT_BRK) {
      ox=x; oy=y;
   }

   state->buttons = GPM_B_LEFT * ((data[0]>>2) & 1)
     + GPM_B_MIDDLE * ((data[0]>>3) & 1)
     + GPM_B_RIGHT * ((data[0]>>4) & 1);

   state->dx = (x-ox)/5; state->dy = (oy-y)/5;
   ox=x; oy=y;
   return 0;
}

