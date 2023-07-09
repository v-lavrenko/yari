#!/bin/tcsh -f

# skip attribute names, except for attribute:true
tr ',' ' ' | sed 's/: *true//g' | sed 's/[^ ]*"://g' | tr -d '"[]{}'
