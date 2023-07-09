#!/usr/bin/env -S gawk -f

BEGIN {if (!f) f = 1; }

{ x = $f; X[n++] = x; sum += x; }

END {
    if (!tag) tag = FILENAME;
    for (i=0; i<n; ++i) { p = X[i] / sum; H -= p * log(p); sp += p; }
    printf "H: %.4f n: %d ∑: %d ∑p: %.2f %s\n", H, n, sum, sp, tag;
}
