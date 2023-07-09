#!/usr/bin/env -S gawk -f

{ 
    r = $1; c = $2; x = $3;
    if (pct) x *= 100;
    N[r,c] += 1;
    S[r,c] += x;
    V[r,c] += x*x;
    R[r]=1; C[c]=1; 
    if (length(r) > wr) wr = length(r);
    if (length(c) > wc) wc = length(c);
}


END {
    if (!fmt) fmt = pct ? "2.0" : "6.4";
    nr = asorti(R);
    nc = asorti(C);
    
    printf "| %"wr"s", ""; # header
    for (j=1; j<=nc; ++j) printf " | %"wc"s", C[j];
    printf " |\n";

    for (i=1; i<=nr; ++i) {
	printf "| %"wr"s", R[i];
	for (j=1; j<=nc; ++j) {
	    r = R[i]; c = C[j];
	    n = N[r,c] ? N[r,c] : 1;
	    if (delta) DELTA = j>1 ? (S[r,c] / n - mean) : 0;
	    mean = S[r,c] / n;
	    var = V[r,c]/n - mean*mean;
	    stdev = sqrt(var>0?var:0);
	    if (pool) printf " | %"fmt"f", (S[r,c] / pool);
	    else      printf " | %"fmt"f", mean;
	    if (delta) printf " %+"delta"f", DELTA;
	    if (n>1 && std) printf " ± %"fmt"f", 2*stdev;
#	    printf " | %"w"."d"f", T[R[r],C[c]];
	}
	printf " |\n";
    }
}
