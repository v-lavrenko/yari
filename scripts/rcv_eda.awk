#!/bin/gawk -f

BEGIN { FS = "\t"; }

$1 && $2 {	
    k = $1; v = $2; # key, value
    if (index($1,"[")) next;
    ++keys[k]; # set of keys
    n = ++nkv[k,v]; # frequency of value (for key)
    if (n == 1) { # first time we see this value
	i = ++nv[k]; # number of values per key
	vals[k,i] = v;	    
    }
}

END {
    for (k in keys) {
	printf "%d %s -> ", nv[k], k;
	if (nv[k] < 10) {
	    for (i = 1; i <= nv[k]; ++i) {
		v = vals[k,i];
		printf " %d:%s", nkv[k,v], v;
	    }
	    print "";
	}
	else print "MANY";
    }
}
