#!/usr/bin/env python3

import csv, sys, os

csvin = csv.reader(sys.stdin)
tsvout = csv.writer(sys.stdout, delimiter='\t', lineterminator='\n')
for row in csvin:
    try:
        tsvout.writerow([s.replace('\n',' ').replace('\t',' ') for s in row])
    except BrokenPipeError:
        sys.stderr.write("BrokenPipeError: exiting\n")
        os._exit(1)
#    print(len(row))
#    
