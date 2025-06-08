#!/usr/local/bin/gawk -f

BEGIN {
    if (!f) f = 1; # field with values
    if (!n) n = 100; # number of bins
}

function toLog(x) { return (x>0) ? log(x) : (x<0) ? -log(-x) : 0; }
function unLog(x) { return (x>0) ? exp(x) : (x<0) ? -exp(-x) : 0; }

{
    x = $f;
    if (L) x = toLog(x);
    if (NR == 1) lo = hi = x;
    X[NR] = x;
    if (x < lo) lo = x;
    if (x > hi) hi = x;
}

END {
    for (i=1; i<=NR; ++i) {
	u = (X[i] - lo) / (hi - lo);
	H[int(n * u)] += 1;
    }
    for (i=0; i<=n; ++i) {
	u = (i / n);
	x = lo + u * (hi - lo);
	if (L) x = unLog(x);
	print x, H[i];
    }
}
