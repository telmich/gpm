
(
   grep -h "^int M_" src/drivers/*/m.c | sed 's/$/;/'
   grep -h "^Gpm_Type" src/drivers/*/i.c | sed 's/$/;/'
   grep -h "^int R_" src/drivers/*/r.c | sed 's/$/;/'
)

