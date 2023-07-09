#!/usr/local/bin/gawk -f

function ln_pick(N,n) { return N*log(N) - n*log(n) - (N-n)*log(N-n); }

# qryid docid score, sorted by decreasing score
{
    ++total[$1];
    if ($1 == $2) found[$1] = total[$1];
}

END {
    if (!ITER) ITER = SELECT ? 30 : 1;
    for (i=1;i<=ITER;++i) {
	nq = eval = 0;
	for (q in found) { 
	    ++nq; 
	    
	    if (MRR) {
		name = "MRR";
		eval += 1 / found[q];
	    }
	    
	    if (RANKS) {
		printf "%3d / %4d %s\n", found[q], total[q], q;
	    }
	    
	    if (SELECT) {
		name = "SELECT@"SELECT;
		for (s=1;s<SELECT;++s) {
		    rank = int(rand()*total[q]+1);
		    if (rank < found[q]) break;
		}
		eval += (s>=SELECT);
	    }
	    
	    if (select) {
		name = "SELECT@"SELECT;
		eval += (1-(found[q]-1)/total[q])**(select-1);
	    }
	    
	}
	if (nq && name)	{
	    acc = eval/nq;
	    if (pct) acc = int(100*acc);
	    print acc, "=", eval, "/", nq, name, "iter", i;
	}
    }
}
