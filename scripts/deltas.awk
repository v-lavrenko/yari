#!/usr/local/bin/gawk -f

BEGIN {
    if (!a) a = 1;
    if (!z) z = 0;
    if (f) a = z = f;
}

NR == 1 {
    if (!z) z = NF;
    for (i=1; i<=(z?z:NF); ++i) prev[i] = first[i] = $i;
}

NR > 1 {
    for (i=1; i<=z; ++i) delta[i] = $i - prev[i];
    for (i=1; i<=z; ++i) prev[i] = $i; 
    for (i=1; i<=z; ++i) $i = delta[i];
    print;
}
