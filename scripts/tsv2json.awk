#!/usr/bin/gawk -f

BEGIN { FS="\t"; id=-1; if (!NL) NL="\n";}

++id == 0 { for (i=1; i<=NF; ++i) F[i] = $i; next; }

{ 
    printf "{%s", NL;
    if (row) printf "\"row\": \"%d\", %s", id, NL;
    for (i=1; i<=NF; ++i) printf "\"%s\": \"%s\"%s %s", F[i], $i, (i<NF?",":""), NL;
    printf "}\n"
}
