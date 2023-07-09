#!/usr/bin/env python3

"""
Plot the value of 'y' as a function of four variables: x1 x2 x3 x4
Input: x1 x2 x3 x4 y from stdin
"""

import re
import sys
import string
import fileinput
import time
import math
from matplotlib.pyplot import *
F = 6
rc("axes", labelsize=F, titlesize=F)
rc("xtick", labelsize=F)
rc("ytick", labelsize=F)
rc("font", size=F)
rc("legend", fontsize=F)

def uniq(L):
    H = {}
    for x in L: H[x] = 1
    return sorted(H.keys())

def val(H,key,default):
    if key not in H: return default
    return H[key]

# file where the result will be saved
if len(sys.argv) > 1: result = sys.argv[1]
else: result = None

X,Y = [], {}
for line in sys.stdin:
    fields = line.split()
    x,y = tuple (fields[-5:-1]), float (fields[-1:][0])
    Y[x] = y
    X += [x]

X = [uniq(x) for x in zip(*X)]

print ("X:")
for x in X: print (x)

#print "Y:"
#for x in Y: print x, "->", Y[x]

nrows,ncols,i,r = len(X[-4]), len(X[-3]), 0, 0
for x1 in X[-4]:
    r += 1
    for x2 in X[-3]:
        M = [[val(Y,(x1,x2,x3,x4),0) for x3 in X[-2]] for x4 in X[-1]]
        #print "M:"
        #for row in M: print row
        i += 1
        subplot (nrows,ncols,i)
        p = plot(M)
        ylim ([min(Y.values()),max(Y.values())])
        xticks (range(len(X[-1])), X[-1])
        if r < nrows: xticks (arange(len(X[-1])), ['' for x in X[-1]])
        if i == 1: legend (p, X[-2])
        title (x1+"-"+x2)
        grid ()

if result is None: show()
else:
    savefig(result)
    print ("Plot saved in:", result)

sys.exit()
