#!/usr/local/bin/gawk -f

BEGIN {
    if (!ranks) ranks = "1,2,5,10,20,50,100,200,500,1000,2000,5000,10000,20000,50000";
    n = split(ranks,a,",");
    for(i=1;i<=n;++i) R[a[i]+0] = 1;
}

NR in R {
    if (f) printf " %d:%s", NR, $f;
    else   print; # whole line
}

END {if (f) print ""; }
