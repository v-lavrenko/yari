#!/usr/bin/gawk -f

### Now why the hell does UNIX join does not work????

BEGIN {

    if (f) f1 = f2 = f;
    if (!f1) f1 = 1;
    if (!f2) f2 = 1;

    if (ARGC < 3) { print "Usage: join [-vf=field] file1 file2"; exit; }

    while (0 < (getline < ARGV[1])) v [$f1] = $0;

    while (0 < (getline < ARGV[2])) if ($f2 in v) print v[$f2], "|", $0;
}
