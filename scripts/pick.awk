#!/usr/bin/env -S gawk -f

BEGIN {
    if (ARGC < 2) {print "Usage: pick A B C D E ... prints random choice"; exit(1);}
    if (0 < ("date '+%N'" | getline nanos)) srand(nanos);
    r = rand();
    n = 1+int((ARGC-1)*r); # uniform over ARGV
    print ARGV[n]; 
}
