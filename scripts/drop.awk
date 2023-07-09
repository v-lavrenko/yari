#!/usr/bin/gawk -f

BEGIN {
    if (!f) f = 1;
    while ((getline<ARGV[1])>0) keep[$f] = 1;
    ARGV[1] = "";
}

!($f in keep)
