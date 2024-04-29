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

#include "math.h"
#include "cluster.h"
#include "hl.h"

// ------------------------- sims -------------------------

/* deprecated: use subset_rows + renumber_cols

// V[i] = vector of D[i].i (from DOCS)
ix_t **doc_vecs(ix_t *D, coll_t *DOCS) {
  ix_t **V = new_vec (0, sizeof(ix_t*)), *d;
  for (d = D; d < D+len(D); ++d) {
    ix_t *vec = get_vec (DOCS, d->i);
    V = append_vec (V, &vec);
  }
  return V; // de-allocate with free_2D(V)
}
*/

float **pairwise_sims (coll_t *A, coll_t *B) {
  uint a, b, nA = num_rows(A), nB = num_rows(B);
  float **S = (float**) new_2D(nA, nB, sizeof(float));
  for (a = 0; a < nA; ++a) {
    ix_t *Va = get_vec_ro (A,a);
    for (b = 0; b < nB; ++b) {
      ix_t *Vb = get_vec_ro (B,b);
      S[a][b] = cosine(Va,Vb);
    }
  }
  return S;
}


/* deprecated: use coll_t version above

float **pairwise_sims (ix_t **A, ix_t **B) {
  uint a, b, nA = len(A), nB = len(B);
  float **S = (float**) new_2D(nA, nB, sizeof(float));
  for (a = 0; a < nA; ++a)
    for (b = 0; b < nB; ++b)
      S[a][b] = cosine(A[a],B[b]);
  return S;
}
*/

float **self_sims (coll_t *D) {
  uint i, j, n = num_rows(D);
  float **S = (float**) new_2D(n, n, sizeof(float));
  for (i = 0; i < n; ++i) {
    ix_t *Vi = get_vec (D,i);
    for (j = 0; j < i; ++j) {
      ix_t *Vj = get_vec_ro (D,j);
      S[j][i] = S[i][j] = cosine(Vi,Vj);
    }
    S[i][i] = 1;
    free_vec (Vi);
  }
  return S;
}

/* deprecated: use coll_t version above

float **self_sims (ix_t **D) {
  uint i, j, n = len(D);
  float **S = (float**) new_2D(n, n, sizeof(float));
  for (i = 0; i < n; ++i) {
    for (j = 0; j < i; ++j)
      S[j][i] = S[i][j] = cosine(D[i],D[j]);
    S[i][i] = 1;
  }
  return S;
}
*/

void show_sims (float **sim) {
  uint r, c, nr = len(sim), nc = len(sim[0]);
  printf("--");
  for (c = 0; c < nc; ++c) printf(" %s%4d%s", fg_RED, c, RESET);
  printf(" --\n");
  for (r = 0; r < nr; ++r) {
    printf("%s%2d%s", fg_RED, r, RESET);
    for (c = 0; c < nc; ++c) printf(" %4.2f", sim[r][c]);
    printf(" %s%2d%s\n", fg_RED, r, RESET);
  }
  printf("--");
  for (c = 0; c < nc; ++c) printf(" %s%4d%s", fg_RED, c, RESET);
  printf(" --\n");
}

// ------------------------- MST -------------------------

jix_t *docs_mst (ix_t *docs, float **sim) {
  jix_t *mst = new_vec (0, sizeof(jix_t));
  uint n = len(docs), *aa, *bb;
  uint *A = new_vec(1, sizeof(uint)), *a; // doc [0] already in MST [0]
  uint *B = new_vec(n-1, sizeof(uint)), *b; // docs [1:n-1] to be added
  for (b = B; b < B+len(B); ++b) *b = b-B+1;
  while (len(B)) { // find most-similar pair aa,bb where:
    for (aa = a = A; a <= A+len(A); ++a) // aa is already in MST
      for (bb = b = B; b <= B+len(B); ++b) // bb not yet in MST
	if (sim[*a][*b] > sim[*aa][*bb]) { aa=a; bb=b; }
    jix_t edge = {docs[*aa].i, docs[*bb].i, sim[*aa][*bb]};
    mst = append_vec(mst, &edge);
    A = append_vec(A,bb); // add bb to MST
    *bb = B[--len(B)]; // remove bb from TODO (replace with last)
  }
  free_vec (A);
  free_vec (B);
  return mst;
}

//digraph {
//    a -> b[label="0.2",weight="0.2"];
//}
void mst_dot (jix_t *mst, hash_t *ID) {
  jix_t *m;
  printf("digraph {\n");
  for (m = mst; m < mst+len(mst); ++m) {
    char *a = id2str(ID, m->i);
    char *b = id2str(ID, m->j);
    printf("%s -> %s [label=\"%.2f\"];\n", a, b, m->x);
    free(a); free(b);
  }
  printf("}\n");
}

// ------------------------- k-means -------------------------

#ifdef KMEANSix

// {j:cluster, i:doc, x:similarity}
jix_t *assign_docs_randomly (uint nd, uint nc) {
  uint d; jix_t *A = new_vec (nd, sizeof(jix_t));
  for (d=0; d < nd; ++d)
    A[d] = (jix_t) {random() % nc, d, 0};
  return A;
}

// A[d] = nearest centroid for d based on similarity matrix
void assign_docs_centroids (jix_t *A, float **DxC) {
  uint d, nd = len(DxC);
  for (d=0; d < nd; ++d) {
    float *sims = DxC[d], *best = maxf(sims); // pointer to max sim
    A[d] = (jix_t) {best-sims, d, *best};
  }
}

// quality: avg similarity of doc to its centroid
float assigned_affinity (jix_t *A) {
  double affinity = 0; jix_t *a;
  for (a = A; a < A+len(A); ++a) affinity += a->x;
  return affinity / len(A);
}

// number of documents assigned to each centroid
uint *assigned_size (jix_t *A, uint nc) {
  jix_t *a; uint *sz = new_vec(nc, sizeof(uint));
  for (a = A; a < A+len(A); ++a) ++sz[a->j];
  return sz;
}

// C[c] = mean of documents D[d] assigned to c
ix_t **mean_centroids (ix_t **D, jix_t *A, uint nc) {
  jix_t *a; uint nd = len(D), na = len(A); 
  assert (na == nd);
  ix_t **C = new_vec(nc, sizeof(ix_t*)), **c;
  for (c = C; c < C+nc; ++c) *c = new_vec (0, sizeof(ix_t));
  for (a = A; a < A+na; ++a) {
    ix_t *old = C[a->j], *doc = D[a->i];
    C[a->j] = vec_add_vec (1, old, 1, doc);
    free_vec(old);
  }
  // divide each centroid by number of assigned docs (avg)
  uint *sz = assigned_size (A, nc), j;
  for (j = 0; j < nc; ++j) vec_x_num (C[j], '/', sz[j]);
  free_vec (sz);
  return C;
}

jix_t *k_means_1 (ix_t **D, uint k, int iter) {
  jix_t *A = assign_docs_randomly (len(D), k);
  while (--iter >= 0) {
    ix_t **C = mean_centroids (D, A, k);
    float **DxC = pairwise_sims (D, C);
    assign_docs_centroids (A, DxC);
    free_2D((void**)DxC);
    free_2D((void**)C);
  }
  return A;
}

// largest word id in any of vectors in collection C
uint max_word_id (ix_t **C) {
  uint id = 0, nc = len(C); ix_t **c;
  for (c = C; c < C+nc; ++c) {
    if (!len(*c)) continue;
    ix_t *last = (*c) + len(*c) - 1;
    id = MAX(id,last->i);
  }
  return id; 
}

// ICF[w] = {i:word, x:number of clusters containing word}
ix_t *inverse_cluster_frequency1 (ix_t **C) {
  uint nc = len(C);
  ix_t **c, *ICF = new_vec (0, sizeof(ix_t)), *w;
  for (c = C; c < C+nc; ++c) {
    ix_t *f = copy_vec (*c), *F = ICF;
    vec_x_num (f, '=', 1);
    ICF = vec_add_vec (1, F, 1, f);
    free_vec (f);
    free_vec (F);
  }
  for (w = ICF; w < ICF+len(ICF); ++w)
    w->x = log ((0.5 + nc) / w->x) / log (1.0 + nc);
  return ICF;
}

void disjoin_centroids (ix_t **C) {
  uint nc = len(C);
  ix_t *icf = inverse_cluster_frequency1 (C);
  ix_t **c;
  for (c = C; c < C+nc; ++c) {
    vec_mul_vec (*c, icf);
  }
  free(icf);
}

void show_assigned (jix_t *_A) {
  jix_t *A = copy_vec(_A);
  jix_t *a = A, *b = A, *end = A+len(A);
  sort_vec (A, cmp_jix);
  while (a < end) {
    while (b < end && b->j == a->j) ++b; // [a:b) are in same cluster
    printf("%s%2d%s", fg_RED, a->j, RESET); // cluster number is a->j
    for (; a < b; ++a) printf(" %d", a->i);
    printf("\n");
    assert (a==b);
  }
  free_vec(A);
}

#endif // KMEANSix

// run K-means over documents in DxW
// store cluster assignments in KxD
// store cluster centroids in KxW
void k_means (coll_t *DxW, uint K, int iter, coll_t **_KxD, coll_t **_KxW) {
  printf("\nk-means: D[%dx%d] -> %d clusters %d iters\n",
	 num_rows(DxW), num_cols(DxW), K, iter);
  uint D = num_rows(DxW); // , W = num_cols(DxW);
  coll_t *KxW = open_coll_inmem(); // cluster centroids
  coll_t *KxD = open_coll_inmem(); // cluster-document assignment
  weigh_mtx_or_vec (DxW, NULL, "L2=1", NULL);
  rand_mtx_sparse  (KxD, K, D, 5); // 5 random docs per cluster
  weigh_mtx_or_vec (KxD, NULL, "L1=1", NULL);
  while (--iter >= 0) {
    rows_x_cols (KxW, KxD, DxW); // centroids = assigned x docs
    rows_x_rows (KxD, KxW, DxW); // centroid-doc similarities
    disjoin_rows (KxD);
    printf ("%d affinity: %.2f\n", iter, sum_sums(KxD,1)/D);
    weigh_mtx_or_vec (KxD, NULL, "uniform", NULL);
  }
  if (_KxD) *_KxD = KxD;
  else free_coll(KxD);
  if (_KxW) *_KxW = KxW;
  else free_coll(KxW);
}

// F[w] = how well word w pinpoints a group
float *inverse_cluster_frequency2 (coll_t *KxW) {
  float *f, N = num_rows (KxW);
  float *F = sum_cols (KxW, 0); // F[w] = #clusters with word w
  for (f = F; f < F+len(F); ++f)
    if (*f) *f = log ((N+0.5) / *f) / log(N+1);
    //if (*f) *f = N / *f;
  return F;
}

// reduce each centroid to most discriminative keywords
void cluster_signatures (coll_t *KxW) {
  float *F = inverse_cluster_frequency2 (KxW);
  uint K = num_rows(KxW), k;
  for (k = 1; k <= K; ++k) {
    ix_t *vec = get_vec (KxW, k);
    vec_x_full (vec, '*', F);
    vec_x_full (vec, '*', F);
    put_vec (KxW, k, vec);
    free_vec (vec);
  }
  free_vec (F);
  disjoin_rows (KxW);
}


// keep only 1st doc in each clump
// replace clump id (j) with number of docs in the clump
void one_per_clump(jix_t *C) {
  jix_t *c, *d, *end = C+len(C);
  uint maxj = 0, j; // how many clumps?
  for (c = C; c < end; ++c) if (c->j > maxj) maxj = c->j;
  uint *seen = new_vec (maxj+1, sizeof(uint));
  for (c = C; c < end; ++c) ++seen[c->j]; // size of each clump
  for (c = C; c < end; ++c) { j = c->j; c->j = seen[j]; seen[j] = 0; }
  for (c = d = C; c < end; ++c) if (c->j) *d++ = *c;
  len(C) = d - C;
  free_vec(seen);
}

// re-order so all els in a group appear consecutively
// original order inside group & 1st el of each group
void group_jix_j (jix_t *src) {
  jix_t *out = new_vec (0, sizeof(jix_t));
  jix_t *s, *t, *end = src+len(src);
  for (s = src; s < end; ++s) {
    if (!s->j) continue; // this element already used
    for (t = s; t < end; ++t) {
      if (t->j != s->j) continue;
      out = append_vec(out, t);
      t->j = 0; // mark this element as used
    }
  }
  assert (len(out) == len(src));
  for (s=src, t=out; s < end; ++t, ++s) *s = *t;
  free_vec(out);
}

// {j:group, i:doc, x:sim} greedy TDT-like thresholding
jix_t *clump_docs (ix_t *D, coll_t *DOCS, char *prm) {
  sort_vec(D, cmp_ix_X);
  float thresh = getprm(prm, "clump=", 1.1);
  uint rerank = getprm(prm, "rerank=", 50);
  jix_t *C = new_vec (0, sizeof(jix_t));
  ix_t *d, **U = new_vec (0, sizeof(ix_t*)), **u;
  for (d = D; d < D+len(D); ++d) {
    ix_t *doc = get_vec_ro (DOCS, d->i), **uEnd = U+len(U);
    for (u = U; u < uEnd; ++u) // look for duplicates
      if (cosine(doc,*u) > thresh) break; // found d ~ *u
    if (u >= uEnd) { // nothing similar to d => new clump
      doc = copy_vec(doc);
      U = append_vec(U, &doc); // cache a copy of vec
    }
    uint clump = (u < uEnd) ? (u-U+1) : len(U);
    jix_t new = {clump, d->i, d->x};
    C = append_vec (C, &new);
    if (len(C) > rerank) break;
  }
  for (u = U; u < U+len(U); ++u) free_vec(*u);
  free_vec (U);
  return C;
}

/*
coll_t *mtx_rowset (coll_t *M, ix_t *R) {
  coll_t *D = open_coll_inmem(); ix_t *r;
  for (r = R; r < R + len(R); ++r) {
  }
  return D;
}
*/

// -------------------- DTree / InfoGain --------------------

// convert frequencies (out of n) into entropies
void binary_entropies (ix_t *F, float n) {
  ix_t *f; for (f = F; f < F+len(F); ++f) {
    double p = f->x / n, z = log(2);
    f->x = - (sqrt(p) * log(p) + (1-p) * log(1-p)) / z;
  }
}

void poisson_pmf (ix_t *F, double mu) {
  ix_t *f;
  for (f = F; f < F+len(F); ++f) {
    double k = f->x;
    f->x = exp(k * log(mu) - mu - lgamma(k+1));
  }
}


void show_vec2 (char *tag, uint K, ix_t *_V, hash_t *H) {
  ix_t *V = copy_vec(_V), *v;
  printf("%s%s%s [%d]:", fg_BLUE, tag, RESET, len(V));
  sort_vec(V, cmp_ix_X);
  if (K > len(V)) K = len(V);
  for (v = V; v < V+K; ++v) {
    char *word = id2str(H, v->i);
    printf (" %s%s%s:%.2f", fg_CYAN, word, RESET, v->x);
    free (word);
  }
  printf("\n");
  fflush(stdout);
  free_vec(V);
}

// pick word that splits docs R in half
// L,R = docs with / without word
// discard words that occur in L
void ig_cluster (coll_t *DxW, hash_t *H, coll_t **_KxD, coll_t **_KxW) {
  coll_t *KxW = open_coll_inmem(); // word representing the cluster
  coll_t *KxD = open_coll_inmem(); // docs assigned to the cluster
  uint nd = num_rows(DxW), K = 0;
  ix_t *R = const_vec (nd,1); // start w all docs
  rows_x_num (DxW, '=', 1); // words occur or not
  while (len(R) && ++K < 10) {
    fprintf(stderr, "-----\n");
    ix_t *W = cols_x_vec (DxW, R); // all words in R (w frequency)
    show_vec2("W", 30, W, H);
    //binary_entropies (W, len(R));
    poisson_pmf (W, 10);            // want word that grabs ~5 docs
    show_vec2("W", 20, W, H);
    trim_vec (W, 1); W->x=1;       // W: word for best split
    show_vec2("W", 20, W, H);
    ix_t *L = rows_x_vec (DxW, W); // L: all docs containing W
    chop_vec(L);
    filter_not (R, L);             // R: remaining docs
    show_vec2("L", 30, L, NULL);
    show_vec2("R", 30, R, NULL);
    ix_t *V = cols_x_vec (DxW, L); // V: all words in L
    vec_x_num(V, '>', 1);          //    that occur at least twice
    chop_vec(V);
    show_vec2("V", 30, V, H);
    filter_rows (DxW, '!', V);     // remove V from consideration
    fflush(stdout);
    put_vec (KxW, K, W);
    put_vec (KxD, K, L);
    free_vecs (W, L, V, (void*)-1);
  }
  free_vec (R);
  if (_KxD) *_KxD = KxD; else free_coll(KxD);
  if (_KxW) *_KxW = KxW; else free_coll(KxW);
}

// -------------------- renumber --------------------

// extract rows of M, renumber them as R[1..n]
coll_t *subset_rows (coll_t *M, ix_t *rows) {
  coll_t *R = open_coll_inmem();
  ix_t *r, *rEnd = rows+len(rows);
  for (r = rows; r < rEnd; ++r) {
    ix_t *row = get_vec_ro (M, r->i); // get row M[r]
    put_vec (R, r-rows+1, row); // renumber to 1..n
  }
  return R;
}

// returns a mapping from columns of R to 1,2...used
uint *column_map (coll_t *R) {
  uint nr = num_rows(R), nc = num_cols(R), r, id;
  uint *map = new_vec (nc+1, sizeof(uint)), used = 0;
  for (r = 1; r < nr; ++r) {
    ix_t *V = get_vec_ro (R, r), *v;
    for (v = V; v < V+len(V); ++v) {
      id = v->i;
      if (!map[id]) map[id] = ++used;
    }
  }
  uint reused = 0; // renumber monotonically
  for (id = 1; id <= nc; ++id)
    if (map[id]) map[id] = ++reused;
  assert (used == reused);
  return map;
}

// return inverse mapping: inv[j]==i iff map[i]==j
uint *invert_map (uint *map) {
  uint i, n = len(map), max = 0;
  for (i=0; i<n; ++i) if (map[i] > max) max = map[i];
  uint *inv = new_vec (max+1, sizeof(uint));
  for (i=0; i<n; ++i) if (map[i]) inv [map[i]] = i;
  return inv;
}

// renumber columns in R using map
// assume R is inmem and map is monotonic (no need to re-sort)
void renumber_cols (coll_t *R, uint *map) {
  assert (R->path == NULL); // R is inmem
  uint nr = num_rows(R), r;
  for (r = 1; r <= nr; ++r) {
    ix_t *V = get_vec_ro (R, r), *v;
    assert (last_id(V) < len(map));
    for (v = V; v < V+len(V); ++v) v->i = map[v->i];
    assert (vec_is_sorted (V));
  }
}

// -------------------- transpose --------------------

// pre-allocate so M[i] = blank vector of size[i]
coll_t *prealloc_inmem (uint *size) {
  coll_t *M = open_coll_inmem(); uint i;
  for (i = 1; i < len(size); ++i) {
    ix_t *vec = const_vec (size[i], 0);
    put_vec (M, i, vec);
    free_vec (vec);
  }
  return M;
}

// simple in-memory transpose
coll_t *transpose_inmem (coll_t *ROWS) {
  uint nr = num_rows(ROWS), nc = num_cols(ROWS), r, c;
  uint *size = len_cols(ROWS); // size of each column
  uint *used = new_vec(nc+1, sizeof(uint));
  coll_t *COLS = prealloc_inmem (size);
  for (r = 1; r <= nr; ++r) { // for each row number r
    ix_t *row = get_vec_ro (ROWS, r), *p, *end = row+len(row);
    assert (last_id(row) <= nc);
    for (p = row; p < end; ++p) { // for each column number p->i
      ix_t *col = get_vec_ro (COLS, p->i); // pre-allocated column
      ix_t *new = col + used[p->i]++; // 1st unused element of col
      *new = (ix_t) {r, p->x};
    }
  }
  for (c = 1; c <= nc; ++c) assert (used[c] == size[c]);
  free_vec(size);
  free_vec(used);
  return COLS;
}

// -------------------- MAIN --------------------

#ifdef MAIN
#define arg(i) ((i < argc) ? argv[i] : NULL)
#define a(i) ((i < argc) ? argv[i] : "")

char *usage =
  "usage: cluster ...\n"
  ;

int main (int argc, char *argv[]) {
  if (argc < 2) return fprintf (stderr, "%s", usage);
  if (!strcmp(a(1),"-parse")) return dump_parsed_qry(a(2));
  return 0;
}

#endif
