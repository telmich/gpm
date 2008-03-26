/* Synaptics TouchPad mouse emulation (Henry Davies) */
static int M_synaptics_serial(Gpm_Event *state,  unsigned char *data)
{
   syn_process_serial_data (state, data);
   return 0;
}

