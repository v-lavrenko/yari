#!/usr/local/bin/gawk -f

BEGIN {
    if (!f) f = 1;
    for (x=1; x<1E9; x*=10) scale[x] = scale[2*x] = scale[3*x] = scale[5*x] = scale[7*x] = 1;
}

{
    sum += $f;
    if (NR in scale) printf "%d %.4f %s\n", NR, sum/NR, $0;
}
