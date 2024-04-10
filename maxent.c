/*

  Copyright (c) 1997-2024 Victor Lavrenko (v.lavrenko@gmail.com)

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

static void setm (coll_t *trg, char eq, coll_t *src) {
  (void) eq;
  uint i = 0, n = num_rows (src);
  while (++i <= n) {
    ix_t *row = get_vec_ro (src, i);
    put_vec (trg, i, row);
  }
}

static void setv (void *trg, char eq, void *src) {
  (void) eq;
  memcpy (trg, src, len(src)*vesize(src));
}

float maxent_update_P (jix_t *P, coll_t *X, coll_t *W) {
  float L = 0; jix_t *p;
  for (p = P; p < P+len(P); ++p) {
    ix_t *doc = get_vec_ro (X,p->i);
    ix_t *Pcd = rows_x_vec (W,doc);
    softmax(Pcd);                     // P(*|d) for all classes
    p->x = Pcd [p->j - 1] . x;        // P(c|d) for correct class
    L += log (p->x);                  // log-likelihood
    free_vec (Pcd);
  }
  return L;
}

static void add_D_to_W (coll_t *W, float *D, uint class, float rate) {
  ix_t *old = get_vec (W,class), *end = old+len(old), *w = old-1;
  while (++w < end) D[w->i] += (1 - rate) * w->x;
  ix_t *new = full2vec_keepzero (D);
  put_vec (W,class,new);
  free_vec (new); free_vec (old);
}

void maxent_update_W (jix_t *P, coll_t *X, coll_t *W, float *D, float rate) {
  jix_t *p, *last = P+len(P)-1;
  for (p = P; p <= last; ++p) {
    ix_t *doc = get_vec_ro (X,p->i), *end = doc+len(doc), *w = doc-1;
    float err = (rate < 1) ? (rate * (1 - p->x)) : 1; // rate=1 => init
    while (++w < end) D[w->i] += err * w->x;
    if ((p == last) || (p[1].j > p->j)) add_D_to_W (W, D, p->j, rate);
  }
}

// W[c x w] ... list of words for class c
// Y[c x d] ... list of docs in class c
// X[d x w] ... list of words present in doc d
void maxent_train (jix_t *P, coll_t *X, coll_t *W, char *prm) {
  char buf[999];
  float faster = getprm(prm,"faster=",1.1), rate = getprm(prm,"rate=",0.1);
  float slower = getprm(prm,"slower=",0.5), ridge = getprm(prm,"ridge=",1);
  uint iterations = getprm(prm,"iterations=",10);
  float L0 = -Infinity, L, A;
  uint iter, nw = num_cols (X);
  float *D = new_vec (nw+1, sizeof (float)); // gradient vector
  jix_t *P0 = copy_vec (P);
  coll_t *W0 = open_coll (fmt(buf,"%s.tmp",W->path),"w+");
  for (iter = 0; iter < iterations; ++iter) {
    //maxent_update_W (P,X,W,D,(i?rate:1)); // 1st iteration => centroid
    jix_t *p, *last = P+len(P)-1;
    for (p = P; p <= last; ++p) { // update D based on P
      ix_t *doc = get_vec_ro (X,p->i), *end = doc+len(doc), *w = doc-1;
      float err = iter ? (rate * (1 - p->x)) : 1; // init on 1st iteration
      while (++w < end) D[w->i] += err * w->x;
      if ((p == last) || (p[1].j > p->j)) { //add_D_to_W (W, D, p->j, rate);
	ix_t *old = get_vec (W,p->j), *end = old+len(old), *w = old-1;
	while (++w < end) D[w->i] += w->x - rate * ridge * w->x;
	ix_t *new = full2vec_keepzero (D);
	put_vec (W,p->j,new);
	free_vec (new); free_vec (old);
      }
    }
    //L = maxent_update_P (P,X,W);
    A = L = 0;
    for (p = P; p <= last; ++p) {
      ix_t *doc = get_vec_ro (X,p->i);
      ix_t *Pcd = rows_x_vec (W,doc);
      softmax(Pcd);                     // P(*|d) for all classes
      ix_t *best = max(Pcd), *real = Pcd + p->j - 1;
      p->x = real->x;                   // P(c|d) for correct class
      L += log (p->x);                  // log-likelihood
      A += (best == real);
      free_vec (Pcd);
    }
    fprintf (stderr, "[%.2f] iteration %2d rate %.3f accuracy %4.1f%% likelihood %f\n",
	     vtime(), iter, rate, (100*A/len(P)), L);
    if (L > L0) { setv(P0,'=',P); setm(W0,'=',W); L0 = L; rate *= faster; }
    else        { setv(P,'=',P0); setm(W,'=',W0); L = L0; rate *= slower; }
  }
  free_vec (P0); free_vec (D); rm_coll (W0);
}

void append_jix (coll_t *c, jix_t *jix) ;
static double classification_error (coll_t *Y, coll_t *_P) {
  jix_t *pred = max_cols (_P); sort_vec (pred, cmp_jix);
  coll_t *P = open_coll (0,0); append_jix (P, pred);
  rows_o_rows (P,Y,'&',P);
  double match = sum_sums(P,1), total = sum_sums(Y,1);
  free_vec(pred); free_coll(P);
  return total - match;
}

static double maxent_likelihood (coll_t *YP) {
  double L = 0; uint r = 0, nr = num_rows (YP);
  while (++r <= nr) {
    ix_t *Y = get_vec_ro (YP,r), *end = Y+len(Y), *y = Y-1;
    while (++y < end) L += log (y->x);
  }
  return L;
}

void split_mtx (coll_t *M, float p, coll_t *A, coll_t *B) ;
void maxent2 (coll_t *W, coll_t *_Y, coll_t *X, char *prm) {
  char *mask = strstr(prm,"mask"), *seed = strstr(prm,"seed"), buf[999];
  float faster = getprm(prm,"faster=",1.1), rate = getprm(prm,"rate=",0.1);
  float slower = getprm(prm,"slower=",0.5), h = getprm(prm,"reg=",0.3);
  float valid = getprm(prm,"valid=",0);
  uint iterations = getprm(prm,"iter=",10), iter = 0;
  uint top = getprm(prm,"top=",0);
  uint voc = getprm(prm,"vocab=",0), vocab = num_cols(X);
  float prune = pow (voc*1./vocab, 1./(iterations-2));
  double L, L0 = -Infinity;
  coll_t *XT = transpose (X);
  coll_t *P  = open_coll (fmt(buf,"%s.P",W->path), "w+");
  coll_t *P0 = open_coll (fmt(buf,"%s.P0",W->path),"w+");
  coll_t *W0 = open_coll (fmt(buf,"%s.W0",W->path),"w+");
  coll_t *Y  = open_coll(0,0), *Z = open_coll(0,0); split_mtx (_Y,valid,Y,Z);
  fprintf (stderr, "%5s %3s %5s %6s %6s %3s %3s\n",
	   "time", "#", "rate", "logL", "vocab", "train", "valid");
  if (!seed) {
    rows_x_cols (W,Y,X);                  // W = Y x X        centroids
    weigh_mtx_or_vec (W, 0, "L2=1", 0);
  }
  while (1) {
    rows_x_cols (P,W,XT);                 // P = W x X'
    //rows_x_rows (P,W,X);
    double AY = classification_error (Y,P);                       // training error
    double AZ = classification_error (Z,P);                       // validation error
    col_softmax (P,P);                    // P = (sofmax P')'
    rows_o_rows (P,Y,'.',P);              // P = Y . P
    L = maxent_likelihood (P);
    fprintf (stderr, "%5.2f %3d %5.3f %6.0f %6d %5.0f %5.0f\n",
	     vtime(), iter, rate, L, vocab, AY, AZ);
    if (++iter > iterations) break;
    if (L < L0) { setm(P,'=',P0); setm(W,'=',W0); L = L0; rate *= slower; }
    else        { setm(P0,'=',P); setm(W0,'=',W); L0 = L; rate *= faster; }
    rows_o_rows (P,Y,'-',P);              // P = Y - P            error
    rows_x_cols (W,P,X);                  // W = P x X         gradient
    if (mask) rows_o_rows (W,W,'&',W0);   // W = W & W0      if masking
    rows_a_rows (W,rate,W,(1-rate*h),W0); // W = W0 + r (W - h W0)
    if (voc) {
      vocab = MAX(voc,vocab*prune);
      trim_mtx (W, vocab);
    }
    if (top) trim_mtx (W,top);
  }
  rm_coll(P); rm_coll(P0); rm_coll(W0); free_coll(XT); free_coll(Y); free_coll(Z);
}

static double maxent_error_binary (ix_t *Y, ix_t *P) {
  ix_t *PY = vec_x_vec (P, '.', Y); // negative if sign(P) != sign(Y)
  uint e = count (PY, '>', 0);
  free_vec(PY); return e;
}

static double maxent_likelihood_binary (ix_t *Y, ix_t *P) {
  ix_t *PY = vec_x_vec (P, '.', Y), *p;
  double L = 0;
  for (p = PY; p < PY+len(PY); ++p)
    L += (p->x > 0) ? log (p->x) : log (1+p->x);
  free_vec(PY); return L;
}

ix_t *maxent (ix_t *Y, coll_t *X, coll_t *XT, char *prm) {
  float faster = getprm(prm,"faster=",1.1), rate = getprm(prm,"rate=",0.1);
  float slower = getprm(prm,"slower=",0.5), h = getprm(prm,"reg=",0.3);
  uint iterations = getprm(prm,"iter=",10), iter = 0;
  ix_t *Y1 = copy_vec (Y); vec_x_num (Y1,'>',0.0); // 1 for pos, 0 for neg
  ix_t *P0 = copy_vec (Y); vec_x_num (P0,'=',0.5); // 0.5 for all docs
  ix_t *W0 = const_vec (0,0);
  double A=0, L=0, L0=-Infinity;
  fprintf (stderr, "%5s %3s %5s %10.4s %5s\n", "time", "#", "rate", "logL", "error");
  while (++iter <= iterations) {
    ix_t *D, *G, *Q, *P, *W, *tmp;
    D = vec_x_vec (Y1, '-', P0);             // D = Y - P0        error for each doc
    G = cols_x_vec (X, D);                   // W = D x X         gradient
    W = vec_add_vec (1-rate*h, W0, rate, G); // W = W0 + r (W - h W0)
    Q = cols_x_vec (XT, W);                  // P = W x X.T
    P = vec_x_vec (Q, '&', Y);               // remove non-training docs
    A = maxent_error_binary (Y,P);           // training error
    vec_x_num (P, 's', 1);                   // P = sigmoid(P)
    L = maxent_likelihood_binary (Y,P);
    fprintf (stderr, "%5.2f %3d %5.3f %10.4f %5.0f\n", vtime(), iter, rate, L, A);
    if (L > L0) { L0 = L; rate *= faster; SWAP(P,P0); SWAP(W,W0);  }
    else        { L = L0; rate *= slower; } // roll-back (ignore updates)
    free_vec(D); free_vec(G); free_vec(W); free_vec(Q); free_vec(P);
  }
  free_vec (Y1); free_vec (P0);
  return W0;
}

void maxent_bin (coll_t *_W, coll_t *_Y, coll_t *X, char *prm) {
  uint c = 0, classes = num_rows(_Y);
  coll_t *XT = transpose (X);
  while (++c <= classes) if (has_vec (_Y,c)) {
      ix_t *Y = get_vec (_Y,c); chop_vec (Y); // all positives / negatives for c
      fprintf (stderr, "class %d: %d+ %d-\n", c, count(Y,'>',0), count(Y,'<',0));
      ix_t *W = maxent (Y, X, XT, prm);
      put_vec (_W,c,W);
      free_vec (W); free_vec (Y);
    }
  _W->cdim = X->cdim;
  free_coll (XT);
}

/////////////////////////////////////////////////////////// PA algorithm (I)

// http://jmlr.org/papers/volume7/crammer06a/crammer06a.pdf
ix_t *passive_aggressive (ix_t *_Y, coll_t *_X, char *prm) {
  float *W = new_vec (1+num_cols(_X), sizeof(float));
  double iters = getprm(prm,"iter=",50), it = 0;
  float C = getprm(prm,"C=",1);
  while (++it <= iters) {
    uint errors = 0;
    ix_t *Y = shuffle_vec(_Y), *d;
    for (d = Y; d < Y+len(Y); ++d) {
      ix_t *X = get_vec_ro (_X, d->i);
      float p = dot_full(X,W), y = d->x, L = MIN(C,1-y*p);
      if (L <= 0) continue; // classified correctly
      float t = y * L / sum2(X); // correct: ||X||^2
      ix_t *w = X-1, *wEnd = X+len(X);
      while (++w < wEnd) W[w->i] += t * w->x;
      ++errors;
    }
    free_vec (Y);
    fprintf (stderr, " %d", errors);
    if (!errors) break; // no further updates possible
  }
  fprintf (stderr, " errors\n");
  ix_t *WW = full2vec (W); free_vec (W);
  return WW;
}

void passive_aggressive_bin (coll_t *_W, coll_t *_Y, coll_t *X, char *prm) {
  uint c = 0, classes = num_rows(_Y);
  while (++c <= classes) if (has_vec (_Y,c)) {
      fprintf (stderr, "class %d:", c);
      ix_t *Y = get_vec (_Y,c); chop_vec (Y); // all positives / negatives for c
      ix_t *W = passive_aggressive (Y, X, prm);
      put_vec (_W,c,W);
      free_vec (W); free_vec (Y);
    }
  _W->cdim = X->cdim;
}

void passive_aggressive_1vR (coll_t *_W, coll_t *_Y, coll_t *X, char *prm) {
  float *S = sum_cols (_Y,1);
  ix_t *N = full2vec (S); vec_x_num (N,'=',1); // negatives (for all classes)
  uint c = 0, classes = num_rows(_Y);
  while (++c <= classes) if (has_vec (_Y,c)) {
      fprintf (stderr, "class %d:", c);
      ix_t *P = get_vec (_Y,c); // positives for c
      ix_t *Y = vec_add_vec (+2, P, -1, N); // +1 for positives, -1 for negatives
      ix_t *W = passive_aggressive (Y, X, prm);
      put_vec (_W,c,W);
      free_vec (W); free_vec (P); free_vec (Y);
    }
  free_vec (S); free_vec (N); //free_coll(P); free_coll(XT);
  _W->cdim = X->cdim;
}

/*
ix_t *passive_aggressive_prune (ix_t *_Y, coll_t *_X, char *prm) {
  ix_t *W = const_vec (num_cols(_X), 0.0000001);
  double iters = getprm(prm,"iter=",10), it = 0;
  double voc = getprm(prm,"vocab=",0), vocab = num_cols(_X);
  double prune = pow (voc/vocab, 1./(iters-2));
  float C = getprm(prm,"C=",1);
  while (++it <= iters) {
    if (it > 1 && voc) trim_vec (W, (vocab = MAX(voc,vocab*prune)));
    uint errors = 0;
    ix_t *Y = shuffle_vec(_Y), *d;
    for (d = Y; d < Y+len(Y); ++d) {
      ix_t *X = get_vec_ro (_X, d->i), *tmp = W;
      float p = dot(X,W), y = d->x, L = MIN(C,1-y*p);
      if (L <= 0) continue; // classified correctly
      if (y*p < 0) ++errors;
      float t = y * L / sum2(X);    // correct: ||X||^2
      X = vec_x_vec (X, '&', W);    // don't update pruned terms
      W = vec_add_vec (1, W, t, X);
      free_vec (X); free_vec (tmp);
    }
    free_vec (Y);
    fprintf (stderr, " %d", errors);
  }
  fprintf (stderr, " errors\n");
  return W;
}
*/


/*
void maxent_update (coll_t *Y, coll_t *X, coll_t *W, char *prm) {
  float rate = 0.1, ridge = 1, faster = 1.1, slower = 0.5;
  float L = 0, oldL = -Infinity, iter = 10;

  uint nc = num_rows(Y), nd = num_rows(X), nw = num_rows(I), c, d, w;
  float rate = 0.1, ridge = 1, L = 0, oldL = -Infinity;
  float *D = new_vec (nw+1, sizeof (float)); // gradient vector
  float L = maxent_predict (Y, X, W, P);
  for (c = 1; c <= nc; ++c) { // compute L + W1
    ix_t *docs = get_vec_ro (Y,c), *d;
    for (d = docs; d < docs+len(docs); ++d) { // gradient for class c
      ix_t *doc = get_vec_ro (X,d->i), *dEnd = doc+len(doc), *w;
      ix_t *hyp = rows_x_vec (W,doc);   // projections to all classes
      softmax (hyp);                    // P(c|d) for all classes
      float Pcd = vec_get (hyp, c);     // P(c|d) for correct class
      float err = d->x * (1 - Pcd);
      for (w = doc; w < dEnd; ++w) D[w->i] += err * w->x;
      L += log (Pcd);
      free_vec (hyp); free_vec (doc);
    }
    rate *= (L < oldL) ? slower : faster;
    ix_t *old = get_vec_ro (W,c), *oEnd = old+len(old), *w;
    for (w = old; w < oEnd; ++w) D[w->i] += (1 - rate) * w->x;
    ix_t *new = full2vec (D); // W = (1-rate) W + rate D
    put_vec (W,c,new);
    free_vec (new); free_vec (old); free_vec (docs);
  }



  free_vec (D);
}
*/
