#!/bin/tcsh -f

# assume stdin = JSON records 1-per-line

cat | gron -s \
| tr '\t\r' '  ' \
| sed 's/ = /\t/' \
| sed 's/;$//' \
| sed 's/^json.[0-9]*.//'
