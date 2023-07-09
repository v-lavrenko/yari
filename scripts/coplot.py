#!/usr/bin/env python3
import numpy as np
import matplotlib.pyplot as plt
import sys

H = sys.stdin.readline().split()
X = np.loadtxt(sys.stdin)
Y,X,H = X[:,0],X[:,1:],H[1:]
Y0,Y1 = np.nonzero(Y==0),np.nonzero(Y==1)
X0,X1 = X[Y0],X[Y1]
nc,nr = X0.shape

f, ax = plt.subplots(nr, nr)

for r in range(nr):
    for c in range(nr):
        if r < c:
#            ax[r,c].loglog(X1[:,c],X1[:,r],'r.',markersize=2)
#            ax[r,c].loglog(X0[:,c],X0[:,r],'b+',markersize=2)
            ax[r,c].plot(X1[:,c],X1[:,r],'ro',markersize=5)
            ax[r,c].plot(X0[:,c],X0[:,r],'b+',markersize=5)
            ax[r,c].grid(True)
            if False and r+1 == c:
                ax[r,c].set_ylabel(H[r])
                ax[r,c].set_xlabel(H[c])
            if r+1 < c:
                ax[r,c].set_yticklabels([])
                ax[r,c].set_xticklabels([])
            if r+1 == c:
                ax[r,c].tick_params(axis='both', which='major', labelsize=6)
        if r == c:
            ax[r,c].text(.5, .5, H[r], horizontalalignment='center', verticalalignment='center', fontsize=14, color='red')
        if r >= c: ax[r,c].axis('off')


#plt.tight_layout()
plt.show()
sys.exit(0)

# for r in range(nr):
#     for c in range(nr):
#         if r < c:
#             plt.subplot(nr,nr,r*nr+c+1)
#             plt.loglog(X0[:,r],X0[:,c],'b+')
#             plt.loglog(X1[:,r],X1[:,c],'rx')
#             plt.grid(True)
#         if r == c:
#             plt.subplot(nr,nr,r*nr+c+1)
#             plt.text(.5, .5, H[r], horizontalalignment='center', verticalalignment='center', fontsize=10, color='red')

# plt.show()

#         if r == c:
#             plt.yscale('log', nonposy='clip')
# #            plt.xscale('log', nonposx='clip')            
#             plt.hist(X0[:,r],color='b',bins=50)
#             plt.hist(X1[:,r],color='r',bins=50)
#         elif r > c:
#             plt.plot(X0[:,c],X0[:,r],'b+')
#             plt.plot(X1[:,c],X1[:,r],'rx')
