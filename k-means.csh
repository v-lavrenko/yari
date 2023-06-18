#!/usr/bin/env -S tcsh -f

set it=10 # number of iterations
set top=0

if ("$1" == "-it") then
    set it=$2
    shift ; shift
endif

if ("$1" == "-top") then
    set top=$2
    shift ; shift
endif

set DOC=$1
set WORD=$2
set K=$3
set DOCxWORD={$DOC}x{$WORD}
set WORDxDOC={$WORD}x{$DOC}
set KxDOC={$K}x{$DOC}
set KxWORD={$K}x{$WORD}

if ("$K" == "") then
    echo "Usage: $0:t [-it 10] [-top 0] DOC WORD K"
    exit
endif

if (! -d $DOC)  echo $DOC must be a Yari dictionary
if (! -d $WORD) echo $WORD must be a Yari dictionary
if (! -d $K) echo $K must be a Yari dictionary
if (! -d $DOCxWORD) echo $DOCxWORD must be a Yari matrix
if (! -d $WORDxDOC) echo $DOCxWORD must be a Yari matrix

if (! -d $DOC || ! -d $WORD || ! -d $K || ! -d $DOCxWORD || ! -d $WORDxDOC) exit

set d=`dict size $DOC`

mtx $KxWORD = rnd:seed=0,top=$top $K $WORD # init: random cluster centroids

foreach ii (`seq $it`)
    mtx $KxDOC = $KxWORD x $WORDxDOC cosine,top=$top # similarity of centroid to docs
    mtx $KxDOC = colmax $KxDOC # each document -> most-similar cluster
    mtx norm $KxDOC \
    | gawk -vd=$d -vii=$ii '{printf "[%d] objective: %.4f\n", ii, $1/d}' \
    | hl 'W:objective'
    mtx $KxDOC = uniform $KxDOC # cluster = unweighted set of docs
    mtx $KxWORD = $KxDOC x $DOCxWORD top=$top # centroid = avg of assigned docs
end
