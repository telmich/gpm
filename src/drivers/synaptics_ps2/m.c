/* Synaptics TouchPad mouse emulation (Henry Davies) */
static int M_synaptics_ps2(Gpm_Event *state,  unsigned char *data)
{
   syn_process_ps2_data(state, data);
   return 0;
}

