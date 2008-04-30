  Improve the regex to match Gpm_Type.
  Rename "func" to "fn" to be gentle to awk; This prevents errors like
  awk: cmd. line:7:                func=substr($0, idx);
  awk: cmd. line:7:                ^ syntax error
  awk: cmd. line:8:                gsub(/\n/, " ", func);
  awk: cmd. line:8:                                ^ syntax error
  awk: cmd. line:8: fatal: 0 is invalid as number of arguments for gsub

Signed-off-by: aldot <rep.dot.nop@gmail.com>
---
 scripts/generate_drivers_header.sh |    8 ++++----
 1 files changed, 4 insertions(+), 4 deletions(-)

diff --git a/scripts/generate_drivers_header.sh b/scripts/generate_drivers_header.sh
index 1a05a8f..34eafd4 100755
--- a/scripts/generate_drivers_header.sh
+++ b/scripts/generate_drivers_header.sh
@@ -39,11 +39,11 @@ eof
    # they have line breaks :-(
    cat drivers/*/i.c |  \
       awk 'BEGIN { RS="{"; }
-            /Gpm_Type \*I_/ {
+            /Gpm_Type[ 	][ 	]*\*I_/ {
                idx=index($0,"Gpm_Type *I_");
-               func=substr($0, idx);
-               gsub(/\n/, " ", func);
-               print func
+               fn=substr($0, idx);
+               gsub(/\n/, " ", fn);
+               print fn
             }'
    grep -h "^int R_"    drivers/*/r.c
 ) | sed -e '/^$/d' -e 's/$/;/' -e 's/  */ /g'
-- 
1.5.5

_______________________________________________
gpm mailing list
gpm@lists.linux.it
http://lists.linux.it/listinfo/gpm
