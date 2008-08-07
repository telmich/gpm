/* retrieve the right table according to the modifier string */
char **twiddler_mod_to_table(char *mod)
{
   struct twiddler_map_struct *ptr;

   int len = strlen(mod);

   if(len == 0)
      return twiddler_map->table;

   for(ptr = twiddler_map; ptr->table; ptr++) {
      if(!strncasecmp(mod, ptr->keyword, len))
         return ptr->table;
   }
   return NULL;
}

