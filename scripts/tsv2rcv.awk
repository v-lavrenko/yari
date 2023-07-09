#!/usr/bin/gawk -f

BEGIN {
    FS = OFS = "\t";
    no = -1;
    if (!id) id = 1; # assume 1st column is row id
}

NR == 1 && H || ($1 == "") { # header row
    if (v) printf "Found headers for %d columns: %s %s ... %s\n", NF-1, $2, $3, $NF > "/dev/stderr";
    for (i=2; i<=NF; ++i) F[i] = $i;
    hdr = NF;
    next;
}

{
    if (hdr && NF != hdr) {
	printf "Skipping line %d: has %d != %d fields\n", NR, NF, hdr > "/dev/stderr";
	next;
    }
    ID = hdr ? $1 : id ? $id : NR;
    beg = hdr ? 2 : 1;
    for (i=beg; i<=NF; ++i) print ID, (hdr ? F[i] : (i-beg+1)), $i;
}

END {
    if (v) printf "Converted %d rows x %d cols\n", (hdr ? NR-1 : NR), (hdr ? hdr-1 : NF) > "/dev/stderr";
}
