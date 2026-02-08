#!/usr/local/bin/gawk -f

BEGIN {
    if (ARGC < 3) { print "Usage: cross.awk file1 file2"; exit; }
}

NR==FNR {file1[$0]; next}

{
    for(line in file1) print line, $0;
}
