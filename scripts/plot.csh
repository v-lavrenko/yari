#!/bin/tcsh -f


gnuplot <<EOF
set term png
set grid
set out "plot.png"
set style fill solid border -1
plot "$1" w boxes
EOF

open plot.png
