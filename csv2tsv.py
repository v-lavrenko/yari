#!/usr/bin/env python3

import csv, sys

csvin = csv.reader(sys.stdin)
tsvout = csv.writer(sys.stdout, delimiter='\t', lineterminator='\n')
for row in csvin:
    tsvout.writerow([s.replace('\n',' ').replace('\t',' ') for s in row])
#    print(len(row))
#    
