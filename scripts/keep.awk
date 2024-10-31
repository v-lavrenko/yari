#!/usr/bin/gawk -f

BEGIN {
    FS = OFS = "\t";
    if (!f) f = 1;
    if (!i) i = f;
    if (!o) o = f;
    while ((getline<ARGV[1])>0) keep[$i] = 1;
    ARGV[1] = "";
}

$o in keep
