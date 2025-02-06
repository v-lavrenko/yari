#!/usr/bin/gawk -f

BEGIN {
    if (!f) f = 1;
    max = log(0); #-9999999999;
    min = -max;
}

{
    ++N;
    x[N] = $f;
    sum += x[N];
    sm2 += x[N]*x[N];
    if (x[N]>max) max=x[N];
    if (x[N]<min) min=x[N];
}

END {
    asort(x);
    NN = N ? N : 1;
    mean = sum/NN;
    stdev = sqrt(sm2/NN - mean*mean);
    pct01 = x[1+int(.01*(N-1))];
    pct05 = x[1+int(.05*(N-1))];
    pct10 = x[1+int(.10*(N-1))];
    pct25 = x[1+int(.25*(N-1))];
    pct50 = x[1+int(.50*(N-1))];
    pct75 = x[1+int(.75*(N-1))];
    pct90 = x[1+int(.90*(N-1))];
    pct95 = x[1+int(.95*(N-1))];
    pct99 = x[1+int(.99*(N-1))];
    if (I) {
	fmt = "%9d: %4.0f ± %5.0f [%4.0f %4.0f %4.0f %4.0f %4.0f |%4.0f| %4.0f %4.0f %4.0f %4.0f %4.0f] %8s\n";
	hmt = "%9s: %4s ± %5s [%4s %4s %4s %4s %4s |%4s| %4s %4s %4s %4s %4s] %4s\n";
    }
    else if (Q) {
	fmt = "%9d: %12.6f ± %12.6f [%8.4f %8.4f %8.4f %8.4f %8.4f |%8.4f| %8.4f %8.4f %8.4f %8.4f %8.4f] %8s\n";
	hmt = "%9s: %12s ± %12s [%8s %8s %8s %8s %8s |%8s| %8s %8s %8s %8s %8s] %4s\n";
    }
    else {
	fmt = "%9d: %10.4f ± %10.4f [%6.2f %6.2f %6.2f %6.2f %6.2f |%6.2f| %6.2f %6.2f %6.2f %6.2f %6.2f] %8s\n";
	hmt = "%9s: %10s ± %10s [%6s %6s %6s %6s %6s |%6s| %6s %6s %6s %6s %6s] %4s\n";
    }
    if (org) { # table for org-mode
	gsub("[][]","",fmt); gsub("[:|±]","",fmt); gsub("  *","|",fmt); fmt="|"fmt;
	gsub("[][]","",hmt); gsub("[:|±]","",hmt); gsub("  *","|",hmt); hmt="|"hmt;
    }
    if (H) printf hmt, "N", "mean", "stdev", "min", "1%", "5%", "10%", "25%", "medi", "75%", "90%", "95%", "99%", "max", "file";
    if (N) printf fmt, N, mean, stdev, min, pct01, pct05, pct10, pct25, pct50, pct75, pct90, pct95, pct99, max, (tag?tag:FILENAME);
    #printf "Count:    %d\n", count;
    #printf "Mean:     %g\n", avg;
    #printf "Variance: %g\n", var;
    #printf "St.dev:   %g\n", var^0.5;
    #printf "Median:   %g\n", x[count/2];
    #printf "Max:      %g\n", max;
    #printf "Min:      %g\n", min;
}
