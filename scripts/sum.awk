#!/usr/bin/gawk -f

BEGIN {if (!f) f = 1; min=999999999; max=-min;}

{
    ++n; sum += $f;
    if ($f < min) min = $f; 
    if ($f > max) max = $f;
}

END {
    sum = K ? rshift(sum,10)"K" : M ? rshift(sum,20)"M" : G ? rshift(sum,30)"G" :  T ? rshift(sum,40)"T" : sum;
    print sum, n, min+0, max+0, (tag?tag:FILENAME)
}
