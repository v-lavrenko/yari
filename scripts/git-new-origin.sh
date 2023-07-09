#!/usr/bin/env tcsh

if ("$1" == "") then
   echo "Usage: $0:t git@github.com:XXX/YYY.git"
   echo "        run inside repo, committed"
   exit
endif

git remote show origin | gawk '/Fetch/ {s=$NF} /pushes/ {print "OLD:", s, $0}'

git remote rename origin __old__
git remote add origin $1
git push -u origin master

git remote show origin | gawk '/Fetch/ {s=$NF} /pushes/ {print "NEW:", s, $0}'

echo "\nConsider doing: git remote rm __old__"
