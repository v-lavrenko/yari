#!/usr/bin/env -S gawk -f

hdr && NR==1 { for (i=1; i<=NF; ++i) C[i] = $i; next; }

num { for (i=1; i<=NF; ++i) if ($i) S[i] += 1; next ; }

min { for (i=1; i<=NF; ++i) S[i] += ($i < S[i] || S[i] == "") ? $i : S[i]; next ; }

max { for (i=1; i<=NF; ++i) S[i] += ($i > S[i] || S[i] == "") ? $i : S[i]; next ; }

{ for (i=1; i<=NF; ++i) S[i] += $i; next ; } # sum is the default

END {
    if (avg) for (i=1; i<=NF; ++i) S[i] /= NR;    
    if (hdr) for (i=1; i<=NF; ++i) print C[i]"\t"S[i];
    else   {
	for (i=1; i<=NF; ++i) {
	    if (S[i]) printf " %.4f", S[i];
	    else printf " %s", $i;
	}
	print "";
    }
}
