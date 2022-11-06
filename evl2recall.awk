#!/usr/bin/gawk -f

#qid   R  3  4  5  6  7  8  9  10  11  12 13 
#14176 10 5 15 19 28 52 72 73 102 146 154  /
#14477 16 2  4 15 20 23 42 46  47  57  65  71  78  79 295 3593 6490 /

BEGIN {
    K = split("10 20 50 100 200 500 1000 2000 5000 10000", rank);
    for (k=1; k<=K; ++k) printf "%6s\t", rank[k]; print "";
}

function dump_and_reset() {
    if (pool) for (k=1; k<=K; ++k) printf "%.4f\t", micro[k]/TotRel;
    else      for (k=1; k<=K; ++k) printf "%.4f\t", macro[k]/NQ;
    print (pool ? TotRel : NQ), filename;
    NQ = TotRel = 0;
    for (k=1; k<=K; ++k) macro[k] = micro[k] = 0;
}

FNR == 1 && NQ { dump_and_reset(); }

$NF == "/" {
    ++NQ; qid=$1; NumRel=$2; i=2; filename = FILENAME; TotRel += NumRel;
    for (k=1; k<=K; ++k) { 
	while (i < NF && $i <= rank[k]) ++i; # 1st index > rank
	macro[k] += (i-3)/NumRel; # recall at rank
	micro[k] += (i-3);
    }
}

END { dump_and_reset(); }
