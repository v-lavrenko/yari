/*
  
  Copyright (c) 1997-2016 Victor Lavrenko (v.lavrenko@gmail.com)
  
  This file is part of YARI.
  
  YARI is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  YARI is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
  License for more details.
  
  You should have received a copy of the GNU General Public License
  along with YARI. If not, see <http://www.gnu.org/licenses/>.
  
*/

#include <math.h>
#include "matrix.h"
#include "svm.h"

uint tri (uint i, uint j) { // dense lower-triangular matrix
  if (i >= j) return i*(i-1)/2 + j;
  else        return j*(j-1)/2 + i;
}

uint trilen (uint n) { return (ceil(sqrt(1+8*n)) - 1) / 2; } // n = i(i+1)/2

static inline void L1 (ix_t *X) { vec_x_num (X,'/',sum(X)); }

// K(i,j) ... kernel (dot-product) of training instances i and j
// target ... +1 if training instance is positive, -1 if negative
// result ... positive / negative weights for training instances
ix_t *svm_weights (ix_t *target, coll_t *K) {
  float rate = 1.0, D = Infinity;
  uint i, n = num_rows(K), iterations = 10; 
  ix_t *R = const_vec(n,0), *oldP=0, *oldN=0;
  ix_t *N = copy_vec(target); vec_x_num (N,'<',0); L1(N);
  ix_t *P = copy_vec(target); vec_x_num (P,'>',0); L1(P);
  while (--iterations > 0) {
    ix_t *sN = rows_x_vec (K, N); // SUM_i N(i) K(j,i) ... avg.sim of j to negatives
    ix_t *sP = rows_x_vec (K, P); // SUM_i P(i) K(j,i) ... avg sim of j to positives
    float PP = dot(P,sP), NN = dot(N,sN), NP = dot(N,sP), PN = dot(P,sN);
    if (PP+NN-NP-PN < D) { // distance decreased
      rate = rate * 1.1; // increase learning rate
      D = PP+NN-NP-PN; 
      free_vec(oldP); oldP=0;
      free_vec(oldN); oldN=0;
    } else { // distance increased => roll back
      rate = rate * 0.5; // decrease learning rate
      free_vec(P); P = oldP; oldP=0;
      free_vec(N); N = oldN; oldN=0;
      continue;
    }
    fprintf (stderr, "[%.2f] left: %2d margin: %.4f rate %.4f\n", vtime(), iterations, D, rate);
    for (i=0;i<n;++i) R[i].x = pow( (sN[i].x + PP) / (sP[i].x + NP), rate);
    P = vec_x_vec (oldP=P, '*', R); L1(P);
    for (i=0;i<n;++i) R[i].x = pow( (sP[i].x + NN) / (sN[i].x + PN), rate);
    N = vec_x_vec (oldN=N, '*', R); L1(N);
  }
  ix_t *W = vec_x_vec (P, '-', N);
  free_vec(P); free_vec(N); free_vec(oldP); free_vec(oldN);
  return W;
}

void svm_train_1v1 (coll_t *W, coll_t *C, coll_t *K) { // all-pairs
  uint i, j, nC = num_rows(C);
  for (i = 1; i <= nC; ++i) {
    ix_t *Ci = get_vec(C,i);
    vec_x_num (Ci,'=',1);
    for (j = 1; j < i; ++j) {
      ix_t *Cj = get_vec(C,j);
      vec_x_num (Cj,'=',1);
      ix_t *Tij = vec_x_vec (Ci,'-',Cj);     free_vec (Cj);
      ix_t *Wij = svm_weights (Tij, K);      free_vec (Tij); 
      put_vec (W, tri(i,j), Wij);            free_vec (Wij); 
    }
    free_vec (Ci);
  }
}

void svm_train_1vR (coll_t *W, coll_t *C, coll_t *K) {
  float *CR = sum_cols(C,1); 
  ix_t *Cr = full2vec(CR);
  vec_x_num (Cr,'=',1);
  uint i, nC = num_rows(C);
  for (i = 1; i <= nC; ++i) {
    fprintf (stderr, "[%.2f] class %d vs rest\n", vtime(), i);
    ix_t *Ci = get_vec(C,i); 
    vec_x_num (Ci,'=',2);
    ix_t *Tir = vec_x_vec (Ci,'-',Cr);       free_vec (Ci);
    ix_t *Wir = svm_weights (Tir, K);        free_vec (Tir);
    //ix_t *Wir = cdescent (Tir, K, 0, "");    free_vec (Tir);
    put_vec (W, i, Wir);                     free_vec (Wir);
  }
  free_vec (CR);
  free_vec (Cr);
}

void svm_train (char *_W, char *_C, char *_K) {
  coll_t *K = open_coll (_K, "r+"); // K[i,j] ... kernel of training instances i,j
  coll_t *C = open_coll (_C, "r+"); // C[c] ... list of positive examples for class c
  coll_t *W = open_coll (_W, "w+"); //
  if (1) svm_train_1vR (W,C,K);
  else   svm_train_1v1 (W,C,K);
  free_coll(K);
  free_coll(C);
  free_coll(W);
}

ix_t *svm_1v1 (ix_t *Y) {
  uint i,j,n = trilen(len(Y));
  ix_t *C = const_vec (n,0);
  //for (i=0; i<len(Y); ++i) Y[i].x = (Y[i].x > 0) ? +1 : -1;
  for (i=1; i<=n; ++i) {
    for (j=1; j<i; ++j) C[i-1].x += Y[tri(i,j)-1].x; // i=+, j=- 
    for (j=n; j>i; --j) C[i-1].x -= Y[tri(j,i)-1].x; // i=-, j=+
  }
  return C;
}

void svm_classify (char *_Y, char *_W, char *_K) {
  coll_t *K = open_coll (_K, "r+"); // K[i,j] ... kernel of testing i to training j
  coll_t *W = open_coll (_W, "r+"); // W ... weights learned by svm_train
  coll_t *Y = open_coll (_Y, "w+");  
  uint n, N = num_rows(K);
  for (n = 1; n <= N; ++n) {
    ix_t *k = get_vec (K,n), *tmp=0;
    ix_t *y = rows_x_vec (W, k);             free_vec(k);
    if (0) {y = svm_1v1(tmp=y);}             free_vec(tmp);
    put_vec (Y,n,y);                         free_vec(y);
  }  
  free_coll(K);
  free_coll(W);
  free_coll(Y);
}

ix_t *rocchio (ix_t *_Y, coll_t *X) {
  ix_t *P = copy_vec(_Y); vec_x_num (P, '>', 0); vec_x_num (P, '/', sum(P));
  ix_t *N = copy_vec(_Y); vec_x_num (N, '<', 0); vec_x_num (N, '/', sum(N));
  ix_t *Y = vec_x_vec (P, '-', N);
  ix_t *W = cols_x_vec (X, Y);
  free_vec (Y); free_vec (P); free_vec (N);
  return W;  
}

//  want: y (w'x + wi*xi) > 1 ... for all x
// pivot: wi = (y - w'x) / xi ... since |y| = 1
// error if (y*xi > 0) & (wi < pivot) ... regardless of sgn(wi)
//          (y*xi < 0) & (wi > pivot) 
// 
ix_t *cdescent (ix_t *_Y, coll_t *XT, ix_t *W, char *prm) {
  float c = getprm(prm,"c=",1); // cost of regularisation
  float p = getprm(prm,"p=",1); // type of regularisation
  uint iterations = getprm(prm,"iter=",2);
  ix_t *w, *d, zero = {0,0};
  uint N = num_cols (XT), V = num_rows (XT);
  if (!W) W = const_vec (V, 0);
  ix_t *_P = cols_x_vec (XT, W); // initial predictions
  float *P = vec2full (_P, N, 0); // predictions 
  float *Y = vec2full (_Y, N, 0); // truth
  float *X = new_vec (N+1, sizeof(float)); // inv list
  float *L = new_vec (N+1, sizeof(float)); // left cumulative loss
  float *R = new_vec (N+1, sizeof(float)); // right cumulative loss
  
  while (iterations-- > 0) {
    for (w = W; w < W+len(W); ++w) { // for each word w
      ix_t *D = get_vec (XT, w->i), *last = D+len(D)-1; // pivot weights
      for (d = D; d <= last; ++d) { uint i = d->i;
	X[i] = d->x; // store X
	P[i]-= w->x * X[i]; // prediction without word w
	d->x = (Y[i] - P[i]) / X[i]; // pivot weight at which Y[i] = P[i]
      }
      D = append_vec (D, &zero); 
      last = D + len(D)-1; // allows us to zero out w
      sort_vec (D, cmp_ix_x); // sort pivots: d1 < d2 < d3 ...
      double L1=0, L2=0, R1=0, R2=0;
      for (d = D; d <= last; ++d) { uint i = d->i; 
	if (Y[i] * X[i] < 0) { // hinge loss
	  L1 += Y[i] * X[i] * d->x;
	  L2 += Y[i] * X[i];
	} 
	L[i] = L1 - L2 * d->x;
      }
      for (d = last; d >= D; --d) { uint i = d->i;
	if (Y[i] * X[i] > 0) {
	  R1 += Y[i] * X[i] * d->x;
	  R2 += Y[i] * X[i];
	}
	R[i] = R1 - R2 * d->x;
      }
      double best = 0, bestLoss = Infinity; 
      for (d = D; d <= last; ++d) { uint i = d->i; 
	double Loss = L[i] + R[i] + c * powa (w->x + d->x, p);
	if (Loss < bestLoss) { bestLoss = Loss; best = d->x; }
      }
      if (0) {
	printf ("%3s %2s %6s %6s %6s %6s %1s %6s %1s\n", 
		"id", "Yi", "W'Xi", "Xi", "v", "^", "?", "pivot", "best");
	for (d = D; d <= last; ++d) { uint i = d->i, b = (d->x==best), l = (X[i]*Y[i]<0), r = (X[i]*Y[i]>0);
	  printf ("%3d %+1.0f %+6.2f %+6.2f %+6.2f %+6.2f %1s %+6.2f %1s\n", 
		  i, Y[i], P[i], X[i], L[i], R[i], (l?"v":r?"^":"-"), d->x, (b?"<-":" "));
	}
      }
      //if (w->x != best) printf ("%1d: %+6.2f -> %+6.2f\n", w->i, w->x, best);
      w->x = best;
      for (d = D; d <= last; ++d) P[d->i] += best * X[d->i]; 
      free_vec (D);
    }
    printf ("-------------------------------- %d iterations left\n", iterations);
  }
  free_vec(_P); free_vec(P); free_vec(Y); free_vec(X); free_vec(L); free_vec(R);
  return W;
}


void cd_train_1vR (char *_W, char *_C, char *_T, char *prm) {
  coll_t *T = open_coll (_T, "r+"); // T[w] ... inverted lists
  coll_t *C = open_coll (_C, "r+"); // C[c] ... class examples
  coll_t *W = open_coll (_W, "w+"); // ... weights
  float *CR = sum_cols(C,1); 
  ix_t *Cr = full2vec(CR);
  vec_x_num (Cr,'=',1);
  uint i, nC = num_rows(C);
  for (i = 1; i <= nC; ++i) {
    fprintf (stderr, "[%.2f] class %d vs rest\n", vtime(), i);
    ix_t *Ci = get_vec(C,i); 
    vec_x_num (Ci,'=',2);
    ix_t *Tir = vec_x_vec (Ci,'-',Cr);       free_vec (Ci);
    ix_t *Wir = cdescent (Tir, T, 0, prm);   free_vec (Tir);
    put_vec (W, i, Wir);                     free_vec (Wir);
  }
  free_vec (CR); free_vec (Cr); free_coll(T); free_coll(C); free_coll(W);
}
