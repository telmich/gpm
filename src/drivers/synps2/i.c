/* synaptics touchpad, ps2 version: Henry Davies */
static Gpm_Type *I_synps2(int fd, unsigned short flags,
           struct Gpm_Type *type, int argc, char **argv)
{
   syn_ps2_init (fd);
   return type;
}


