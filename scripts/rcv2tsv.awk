#!/usr/bin/env -S gawk -f

BEGIN { FS = OFS = "\t"; }

{
    if (cat) data[$1,$2] = data[$1,$2]""$3;
    else if (add) data[$1,$2] = data[$1,$2]+$3;
    else data[$1,$2] = $3;
    rows[$1] = 1;
    cols[$2] = 1;
}

END {
    nr = asorti(rows); nc = asorti(cols);
    printf "#";
    for (c=1; c<=nc; ++c) printf "\t%s", cols[c];
    printf "\n";
    for (r=1; r<=nr; ++r) {
	printf "%s", rows[r];
	for (c=1; c<=nc; ++c) printf "\t%s", data[rows[r],cols[c]];
	printf "\n";
    }
}
