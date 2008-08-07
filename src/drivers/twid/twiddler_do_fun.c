int twiddler_do_fun(int i)
{
   twiddler_active_funs[i].fun(twiddler_active_funs[i].arg);
   return 0;
}
