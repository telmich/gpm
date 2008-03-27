static Gpm_Type* I_empty(int fd, unsigned short flags,
    struct Gpm_Type *type, int argc, char **argv)
{
    if (check_no_argv(argc, argv)) return NULL;
    return type;
}
