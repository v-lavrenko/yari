#!/usr/bin/env -S gawk -f

BEGIN {
    OFS = FS = "\t"; # asume TSV
    if (!f) f = 1; # melt from this field
    if (!hop) hop = 1; # stride hop columns into one
    if (!pfx) pfx = ""; # prepend pfx to column names
}

function slice(i,n,  x,j) {
    x = $i;
    for (j=i+1; j<i+n; ++j) x = x"\t"$j;
    return x;
}

NR==1 { for (i=f; i<=NF; ++i) C[i] = pfx""$i; }

hop>1 && NR>1 {
    prefix = slice(1,f-1);
    for (i=f; i<=NF; i+=hop) print prefix, C[i], slice(i,hop);
    next;
}

NR > 1 {
    prefix = $1;
    for (i=2; i<f; ++i) prefix = prefix"\t"$i;
    for (i=f; i<=NF; ++i) if ($i) print prefix, C[i], $i;
}
