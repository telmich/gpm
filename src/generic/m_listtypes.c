int M_listTypes(void)
{
   Gpm_Type *type;

   printf(GPM_MESS_VERSION "\n");
   printf(GPM_MESS_AVAIL_MYT);
   for (type=mice; type->fun; type++)
      printf(GPM_MESS_SYNONYM, type->repeat_fun?'*':' ',
      type->name, type->desc, type->synonyms);

   putchar('\n');

   return 1; /* to exit() */
}

