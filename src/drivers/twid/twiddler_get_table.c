/* This returns the table to use */
static inline char **twiddler_get_table(unsigned long message)
{
   unsigned long mod = message & TW_ANY_MOD;

   struct twiddler_map_struct *ptr;

   for(ptr = twiddler_map; ptr->table; ptr++)
      if(ptr->modifiers == mod)
         return ptr->table;
   return NULL;
}

