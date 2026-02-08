#!/usr/bin/env -S gawk -f

### Now why the hell does UNIX join not work????

BEGIN {

    if (f) f1 = f2 = f;
    if (!f1) f1 = 1;
    if (!f2) f2 = 1;
    if (!sep) sep = "\t";
    
    if (ARGC < 3) { print "Usage: join [-vf=1] [-vsep='\t'] [-vlegacy=0] file1 file2"; exit; }

    if (legacy) { # join every line of file2 to last matching line of file1
	while (0 < (getline < ARGV[1])) v [$f1] = $0;
	while (0 < (getline < ARGV[2])) if ($f2 in v) print v[$f2]""sep""$0;
    } else { # outer join: every line of file2 to every matching line of file1
	while (0 < (getline < ARGV[1])) { i = ++n[$f1];  v[$f1,i] = $0; }
	while (0 < (getline < ARGV[2])) 
	    for (i=1; i<=n[$f2]; ++i)
		print v[$f2,i]""sep""$0;
    }
}
