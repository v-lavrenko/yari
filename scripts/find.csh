#!/bin/tcsh -f

if ("$1" == "") then
    echo "usage: find.csh str"
    exit 1
endif

echo */$1
echo */*/$1
echo */*/*/$1
echo */*/*/*/$1
echo */*/*/*/*/$1
echo */*/*/*/*/*/$1
