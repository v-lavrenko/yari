#!/usr/bin/python

import sys
import numpy as np
import matplotlib.mlab as mlab
import matplotlib.pyplot as plt

x = np.loadtxt(sys.stdin)

# the histogram of the data
n, bins, patches = plt.hist(x, 10, normed=0, facecolor='green', alpha=0.75)

plt.ylabel('Frequency')
plt.grid(True)
plt.show()
