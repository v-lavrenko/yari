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
#include "bitvec.h"
#include "matrix.h"
#include "hash.h"
#include "textutil.h"

jix_t *ix2jix (uint j, ix_t *ix) {
  ix_t *v; jix_t *jix = new_vec (0, sizeof(jix_t));
  for (v = ix; v < ix+len(ix); ++v) {
    jix_t new = {j, v->i, v->x};
    jix = append_vec (jix, &new);
  }
  return jix;
}

uint jix_is_sorted (jix_t *vec) {
  jix_t *v, *end = vec + len(vec);
  for (v = vec+1; v < end; ++v) if (v->j < (v-1)->j) return 0;
  return 1;
}

uint vec_is_sorted (ix_t *V) {
  ix_t *v = V, *end = V + len(V);
  while (++v < end) if (v->i < (v-1)->i) return 0;
  return 1;
}

jix_t *scan_jix (FILE *in, uint maxlen, hash_t *rows, hash_t *cols) {
  jix_t *buf = new_vec (0, sizeof(jix_t)), new = {0,0,0};
  char line[1000];
  uint fskip=0, rskip=0, cskip=0, vskip=0;
  while (fgets (line, 999, in)) {
    if (!strcmp(line,"# END\n")) break; // end of block
    if (*line == '#') continue; // skip over comments
    char *row = tsv_value(line,1);
    char *col = tsv_value(line,2);
    char *val = tsv_value(line,3);
    if (!row || !col || !val) {
      if (++fskip<9) fprintf (stderr, "cannot parse: %100.100s...\n", line);
      free(row); free(col); free(val);
      continue;
    }
    new.j = rows ? key2id(rows, row) : atol(row); // row id -> integer
    new.i = cols ? key2id(cols, col) : atol(col); // column id -> integer
    new.x = atof(val);
    if      (!new.j) {if (++rskip<9) fprintf(stderr, "skipping row [%s] %s", row, line);}
    else if (!new.i) {if (++cskip<9) fprintf(stderr, "skipping col [%s] %s", col, line);}
    //else if (!new.x) { if (++vskip<9) fprintf (stderr, "skipping zero val %s", line); }
    else buf = append_vec (buf, &new); // keep zero values
    free(row); free(col); free(val);
    if (maxlen && len(buf) >= maxlen) break;
  }
  sort_vec (buf, cmp_jix); // rsort?
  if (fskip || rskip || cskip)
    fprintf (stderr, "skipped posts: %d format, %d row, %d col, %d val\n", fskip, rskip, cskip, vskip);
  return buf;
}

jix_t *coll_jix (coll_t *c, uint num, uint *id) {
  jix_t *buf = new_vec (num, sizeof(jix_t)), *bEnd = buf+num, *b = buf;
  for (; *id <= nvecs(c); ++*id) {
    ix_t *vec = get_vec (c,*id), *v;
    if (len(vec) > bEnd - b) { free_vec(vec); break; }
    for (v = vec; v < vec + len(vec); ++v) {
      b->j = *id;
      b->i = v->i;
      b->x = v->x;
      ++b; }
    free_vec (vec);
  }
  return resize_vec (buf, b - buf);
}

void transpose_jix (jix_t *vec) {
  uint tmp; jix_t *v, *end = vec + len(vec); // swap i <=> j
  for (v = vec; v < end; ++v) { tmp = v->i; v->i = v->j; v->j = tmp; }
}

ix_t *next_in_jix (jix_t *jix, jix_t **last) {
  if (!jix || !last) return NULL;
  if (!*last || *last == jix) *last = jix-1; // start from the beginning
  jix_t *r = *last+1, *s = r, *t, *end = jix + len(jix);
  while ((s < end) && (s->j == r->j)) ++s; // [r..s) = same vec
  if (s == r) return NULL; // empty result
  ix_t *vec = new_vec (0, sizeof(ix_t)); // old vec
  for (t = r; t < s; ++t) {
    ix_t new = {t->i, t->x};
    vec = append_vec (vec, &new);
  }
  *last = s-1;
  return vec;
}

void append_jix (coll_t *c, jix_t *jix) {
  assert (jix_is_sorted (jix));
  jix_t *r = jix, *s = jix, *t, *end = jix + len(jix);
  while (r < end) {
    for (s = r+1; (s < end) && (s->j == r->j); ++s); // r..s = same vec
    ix_t *vec = get_or_new_vec (c, r->j, sizeof(ix_t)); // old vec
    for (t = r; t < s; ++t) {
      ix_t new = {t->i, t->x};
      vec = append_vec (vec, &new);
    }
    put_vec (c, r->j, vec);
    free_vec (vec);
    r = t;
  }
}

void chop_jix (jix_t *jix) {
  jix_t *j, *k, *end = jix+len(jix);
  for (j = k = jix; j < end; ++j) if (j->j && j->i && j->x) *k++ = *j;
  j = resize_vec (jix, k - jix);
  assert (j == jix);
}

void uniq_jix (jix_t *vec) {
  if (!vec || !len(vec)) return;
  jix_t *a, *b, *end = vec + len(vec);
  for (a = b = vec; b < end; ++b) {
    if (a->i == b->i && a->j == b->j) continue;
    else if (++a < b) *a = *b;
  }
  len(vec) = a - vec + 1;
}

void put_vec_write (coll_t *c, uint id, void *vec) ;
uint mtx_append (coll_t *M, uint id, ix_t *vec, char how) {
  if (!M || !id || !vec) return 0; // nothing to do
  if (!has_vec (M,id)) { put_vec_write (M, id, vec); return 1; } // no conflict
  if (how == 'r')      { put_vec_write (M, id, vec); return 1; } // replace
  if (how == 's')                                    return 0; // skip
  ix_t *old = get_vec (M,id), *new = 0; uint ok = 0;
  if (how == 'l' && len(vec) > len(old)) { // longer
    put_vec_write (M, id, vec);
    ok = 1;
  }
  if (how == '+' || how == '|' || how == '&') { // join
    new = vec_x_vec (old, how, vec);
    put_vec_write (M, id, new);
    ok = 1;
  }
  free_vec (old); free_vec (new);
  return ok;
}

void scan_mtx_rcv (FILE *in, coll_t *M, hash_t *R, hash_t *C, char how, char verb) {
  ix_t *vec;
  if (!in || !M) return;
  while (1) {
    jix_t *buf = scan_jix (in, 0, R, C), *last = NULL;
    if (!len(buf)) { free_vec (buf); break; }
    while ((vec = next_in_jix (buf, &last))) {
      mtx_append (M, last->j, vec, how);
      if (verb) fprintf (stderr, "added %s [%d]\n", id2key(R,last->j), len(vec));
      free_vec (vec);
    }
    fprintf (stderr, "[%.0fs] added %d cells how:%c -> %d rows, %d cols\n", vtime(), len(buf), how, num_rows(M), num_cols(M));
    free_vec (buf);
  }
}

void show_jix (jix_t *J, char *tag, char how) {
  jix_t *C = copy_vec (J), *c = C, *d, *end = C+len(C);
  sort_vec (C, cmp_jix_jX);
  printf("\n%s\n", tag);
  if (how == '*') {
    for (c = C; c < end; ++c)
      printf("\t%d %d %.2f\n", c->j, c->i, c->x);
  }
  if (how == 'i') {
    while (c < end) {
      for (d = c; d < end && d->j == c->j; ++d); // [c..d) same group
      printf("\t%d [%ld]", c->j, d-c);
      while (c < d) printf(" %d", c++->i);
      printf("\n");
    }
  }
  free_vec (C);
}

void show_vec (ix_t *vec, char what) {
  ix_t *v = vec-1, *end = vec+len(vec);
  switch (what) {
  case 'i': while (++v < end) printf (" %d", v->i);   break;
  case '0': while (++v < end) printf (" %.0f", v->x); break;
  case '2': while (++v < end) printf (" %.2f", v->x); break;
  case '4': while (++v < end) printf (" %.4f", v->x); break;
  case '*': while (++v < end) printf (" %d:%.4f", v->i, v->x); break;
  }
  printf("\n");
}

void dedup_vec (ix_t *vec) { // keep 1st occurrence of every id
  if (!vec || !len(vec)) return;
  ix_t *a = vec-1, *b = vec, *end = vec + len(vec);
  while (++a < end) if (a->i > b->i) *++b = *a;
  a = resize_vec (vec, b - vec + 1);
  assert (a == vec);
}

float aggr_avg (ix_t *A, ix_t*b) { return ((b-A) * A->x + b->x) / (b-A+1); }

void aggr_vec (ix_t *vec, char aggr) {
  //printf ("aggr: %c\n", aggr);
  if (!vec || !len(vec)) return;
  ix_t *a = vec, *b = vec, *end = vec + len(vec);
  switch(aggr) { // min Max sum avg last
  case 'm': while (++b < end) { if (a->i == b->i) a->x = MIN(a->x,b->x); else a=b; } break;
  case 'M': while (++b < end) { if (a->i == b->i) a->x = MAX(a->x,b->x); else a=b; } break;
  case 's': while (++b < end) { if (a->i == b->i) a->x = a->x + b->x;    else a=b; } break;
  case 'a': while (++b < end) { if (a->i == b->i) a->x = aggr_avg(a,b);  else a=b; } break;
  case 'l': while (++b < end) { if (a->i == b->i) a->x = b->x;           else a=b; } break;
  }
  dedup_vec (vec);
}

void uniq_vec (ix_t *vec) {
  if (!vec || !len(vec)) return;
  ix_t *a, *b, *end = vec + len(vec);
  for (a = b = vec; b < end; ++b) {
    if (a == b) continue;
    if (a->i == b->i) a->x += b->x;
    else if (++a < b) *a = *b;
  }
  b = resize_vec (vec, a - vec + 1);
  assert (b == vec);
}

void sort_uniq_vec (ix_t *vec) {
  sort_vec (vec, cmp_ix_i);
  uniq_vec (vec);
}

void chop_vec (ix_t *vec) {
  ix_t *a, *b, *end = vec + len(vec);
  for (a = b = vec; b < end; ++b) if (b->i && b->x) *a++ = *b;
  b = resize_vec (vec, a - vec);
  assert (b == vec);
}

void drop_vec_el (ix_t *vec, uint el) {
  ix_t *v = vec-1, *end = vec + len(vec);
  while (++v < end) if (v->i == el) v->x = 0;
  chop_vec (vec);
}

void sparse_vec (ix_t *vec, float zero) {
  ix_t *v = vec-1, *end = vec + len(vec);
  while (++v < end) if (v->x == zero) v->x = 0;
  chop_vec (vec);
}

ix_t *dense_vec (ix_t *vec, float zero, uint n) {
  if (!n) n = last_id(vec);
  ix_t *dense = const_vec (n, zero);
  ix_t *v = vec-1, *end = vec + len(vec);
  while (++v < end) dense [v->i-1] = *v;
  return dense;
}

// return i'th half-overlapping passage of size w
ix_t *vec2psg (ix_t *V, uint i, uint w) {
  uint beg = i*w/2, L = len(V);
  if (beg + w/2 > L) return NULL;
  if (beg + w > L) w = L - beg;
  ix_t *psg = new_vec (w, sizeof(ix_t));
  memcpy (psg, V+beg, w * sizeof(ix_t));
  vec_x_num (psg, '=', 1);
  sort_uniq_vec (psg);
  return psg;
}

//         ________0________
//     ____1____       ____2____
//   __3__   __4__   __5__   __6__
//   7   8   9  10   11 12   13 14
#define heap_parent(i) (((i)-1)>>1)
#define heap_left(p)  (((p)<<1)+1)
#define heap_right(p) (((p)<<1)+2)

void heapify_up (ix_t *H, uint i) { // max-heap
  ix_t tmp;
  while (i > 0) {
    uint p = heap_parent(i);
    if (H[p].x >= H[i].x) break; // parent >= child: done
    SWAP(H[i],H[p]);
    i = p;
  }
}

void heapify_down (ix_t *H, uint i, uint N) { // max-heap
  ix_t tmp;
  if (N < 2) return;  // singleton => heap
  uint last_parent = heap_parent(N-1);
  while (i <= last_parent) {
    uint L = heap_left(i), R = L+1; // left and right children
    uint c = (R<N && H[R].x > H[L].x) ? R : L; // larger child
    if (H[c].x <= H[i].x) break; // heap preserved
    SWAP(H[i],H[c]);
    i = c;
  }
}

void build_heap (ix_t *H, uint N) {
  if (N < 2) return; // singleton => heap
  uint last = heap_parent(N-1), d;
  for (d = 0; d <= last; ++d) {
    heapify_down (H,last-d,N);
    //printf("[%d] ",last-d);
    //show_vec(H,'2');
  }
}

ix_t *heap_push (ix_t *H, ix_t new) {
  H = append_vec (H,&new);
  heapify_up (H,len(H)-1);
  return H;
}

ix_t heap_pop (ix_t *H) {
  ix_t result = H[0];
  H[0] = H[--len(H)];
  heapify_down (H,0,len(H));
  return result;
}

uint isa_heap (ix_t *H) {
  uint i, n = len(H); // auto-handles the case of n < 2
  for (i=1; i<n; ++i) {
    uint p = heap_parent(i);
    if (H[i].x > H[p].x) return 0; // child > parent => false
  }
  return 1; // every child <= parent
}

/*
void topKadd (ix_t *H, uint K, ix_t new) {
  uint n = len(H);
  if (n < K) { // fewer than K elements => add
    ++len(H);
    H[n] = new;
    heapify_up (H,n);
  }
  else if (new.x > H[1].x) { // bigger than min of top K => replace
    H[1] = new;
    heapify_do (H,1,n);
  }
}
*/

inline float medianof3 (float a, float b, float c) {
  return (a < b) ? ((b < c) ? b :     // a < b < c
		    (a < c) ? c : a)  // a < c < b   or   c < a < b
    :              ((b > c) ? b :     // a > b > c
		    (a > c) ? c : a); // a > c > b   or   c > a > b
}

inline ix_t *medix3 (ix_t *A, ix_t *B, ix_t *C) {
  return (A->x < B->x) ? ((B->x < C->x) ? B :     // a < b < c
			  (A->x < C->x) ? C : A)  // a < c < b or c < a < b
    :                    ((B->x > C->x) ? B :     // a > b > c
			  (A->x > C->x) ? C : A); // a > c > b or c > a > b
}


void assert_partition (ix_t *beg, ix_t *pivot, ix_t *last) {
  ix_t *l, *r;
  for (l = beg; l < pivot; ++l) assert (l->x >= pivot->x);
  for (r = last; r > pivot; --r) assert (r->x <= pivot->x);
}

ix_t *partition (ix_t *a, ix_t *z) {
  ix_t *pivot = a + rand() % (z - a), tmp, *i;
  SWAP(*pivot,*z);
  for (i = a-1; a < z; ++a) if (a->x < z->x) { ++i; SWAP(*i,*a); }
  ++i; SWAP(*i,*z);
  return i;
}

void quickselect (ix_t *A, ix_t *Z, uint K) {
  ix_t *a = A, *z = Z;
  while (1) {
    ix_t *m = partition(a,z);
    if (m - a < K) { K -= (m-a); a = m; }
    else           { z = m; }
  }
}

void qselect (ix_t *X, int k) { // reorder X to have top k elements first
  uint n = ABS(k), top = k>0, bot = k<0;
  if (len(X) <= n) return;
  ix_t *beg = X, *last = X+len(X)-1, *a, *b, tmp;
  while (last > beg) {
    ix_t *pivot = beg + rand() % (last-beg); // faster than medix3
    SWAP(*pivot,*last);
    for (a = b = beg; a < last; ++a)
      if ((top && a->x > last->x) ||
	  (bot && a->x < last->x) ||
	  (a->x == last->x && a->i < last->i)) { SWAP(*a,*b); ++b; }
    SWAP(*last,*b); // beg...b-1 > b >= b+1...last
    if (b == X+n) break;
    else if (b > X+n) {last = b-1; }
    else if (b < X+n) { beg = b+1; }
    else break;
  }
}

/*
void qselect_test (char *prm) {
  ix_t *vec) { // qselect test
  uint tries = 10, n=len(vec);
  while (--tries > 0) {
    uint k = random() % n;
    if (!k) continue;
    ix_t *v = vec-1, *m = vec;
    qselect (vec,k);
    while (++v < vec+k) if (v->x < m->x) m = v; // find min
    while (++v < vec+n) if (v->x > m->x) {
	fprintf (stderr, "\nERROR: %d %.4f > %.4f %d k=%d/%d row=%d/%d\n",
		 v->i, v->x, m->x, m->i, k, n, id, nv);
	break;
      }
  }
}
*/

void trim_vec_sort (ix_t *vec, uint k) {
  if (len(vec) <= k) return;
  sort_vec (vec, cmp_ix_X);
  len(vec) = k;
}

void trim_vec_qsel (ix_t *vec, uint k) {
  if (len(vec) <= k) return;
  qselect (vec, k);
  len(vec) = k;
}

void trim_vec_heap (ix_t *vec, uint k) {
  if (len(vec) <= k) return;
  build_heap(vec,len(vec));
  len(vec) = k; // <<< WRONG! pop k times
}

void test_trim_vec (char *prm) {
  char *_sort = strstr(prm,"sort");
  char *_qsel = strstr(prm,"qsel");
  char *_heap = strstr(prm,"heap");
  char *_int = strstr(prm,"int");
  char *_const = strstr(prm,"const");
  char *_incr = strstr(prm,"incr");
  char *_decr = strstr(prm,"decr");
  uint n = getprm(prm,"n=",1000);
  uint k = getprm(prm,"k=",100);
  uint r = getprm(prm,"r=",1E8/n), i;
  ix_t **vecs = new_vec(r,sizeof(ix_t*));
  for (i=0; i<r; ++i) {
    ix_t *V = rand_vec_exp (n), *v=V-1, *end = V+len(V);
    if (_int)   while (++v < end) v->x = ceilf (v->x);
    if (_const) while (++v < end) v->x = 1;
    if (_incr)  while (++v < end) v->x = v-V;
    if (_decr)  while (++v < end) v->x = V+n-v;
    vecs[i] = V;
  }
  vtime();
  for (i=0; i<r; ++i) {
    if (_sort) trim_vec_sort(vecs[i],k);
    if (_qsel) trim_vec_qsel(vecs[i],k);
    if (_heap) trim_vec_heap(vecs[i],k);
  }
  printf ("%.3f %s\n", vtime(), prm);
}

void trim_vec (ix_t *X, int k) {
  return trim_vec2(X,k);
  //qselect (X, k);
  //len(X) = ABS(k);
  //sort_vec (X, cmp_ix_i);
}

void nksample (ix_t *X, uint n, int k) { // top k elements + random n-k
  if (!X || n >= len(X)) return;
  qselect (X, k);
  ix_t *a = X+k-1, *end = X+len(X);
  while (++a < X+n) {
    ix_t *b = a + rand() % (end - a), tmp;
    if (a < b) SWAP(*a,*b);
  }
  len(X) = n;
}

void trim_vec1 (ix_t *X, int k) {
  uint n = ABS(k), top = k>0, bot = k<0;
  if (len(X) <= n) return;
  ix_t *beg = X, *last = X+len(X)-1, *a, *b, tmp;
  while (last > beg) {
    ix_t *pivot = beg + rand() % (last-beg); // faster than medix3
    SWAP(*pivot,*last);
    for (a = b = beg; a < last; ++a)
      if ((top && a->x > last->x) ||
	  (bot && a->x < last->x) ||
	  (a->x == last->x && a->i < last->i)) { SWAP(*a,*b); ++b; }
    SWAP(*last,*b); // beg...b-1 > b >= b+1...last
    if (b == X+n) break;
    else if (b > X+n) {last = b-1; }
    else if (b < X+n) { beg = b+1; }
    else break;
  }
  len(X) = n;
  sort_vec (X, cmp_ix_i);
}

void trim_vec2 (ix_t *vec, int k) {
  uint n = ABS(k), top = k>0;
  if (len(vec) <= n) return;
  sort_vec (vec, (top ? cmp_ix_X : cmp_ix_x));
  ix_t *tmp = resize_vec (vec, n);
  assert (tmp == vec);
  sort_vec (vec, cmp_ix_i);
}

void trim_mtx (coll_t *M, int k) {
  uint i = 0, n = num_rows (M);
  while (++i <= n) {
    ix_t *vec = get_vec (M,i);
    trim_vec (vec, k);
    put_vec (M,i,vec);
    free_vec (vec);
  }
}

float max_of_k_probes (ix_t *vec, int k) {
  uint n = len(vec);
  float sup = vec->x;
  while (--k >= 0) {
    float x = vec [rand() % n] . x;
    if (x > sup) sup = x; }
  return sup;
}

// keep top n elements from vec, returns < n elements w.p. 1-p
void ptrim (ix_t *vec, uint n, float p) {
  float N = len(vec), k = log (p) / log (1-n/N);
  float sup = max_of_k_probes (vec, (int)k);
  ix_t *a = vec, *b = vec-1, *end = vec+len(vec);
  while (++b < end) if (b->x >= sup) *a++ = *b;
  len(vec) = a - vec;
  //printf("ftrim: want %4d out of %7.0f: sample %4.0f, get %5d over %.4f\n", n, N, k, vcount(vec), sup);
}

// samples k random integers in [0..M), or unrestricted if M == 0.
uint *random_ints(uint k, uint M) {
  uint *R = new_vec(k, sizeof(uint)), i;
  for (i = 0; i < k; ++i) {
    R[i] = (uint)random();
    if (M) R[i] = R[i] % M;
  }
  return R;
}

// samples k integers in [0..M) without replacement.
uint *sample_ints(uint k, uint M) {
  assert (k + 10 < M);
  uint *result = new_vec(0, sizeof(uint));
  char *SEEN = bit_vec(M);
  while (len(result) < k) {
    uint r = random() % M;
    if (bit_is_1(SEEN,r)) continue;
    result = append_vec(result, &r);
    bit_set_1(SEEN,r);
  }
  free_vec(SEEN);
  return result;
}

ix_t *shuffle_vec (ix_t *V) {
  uint i, n = len(V);
  ix_t *permute = rand_vec (n);
  sort_vec (permute, cmp_ix_x);
  ix_t *U = new_vec (n, sizeof(ix_t));
  for (i = 0; i < n; ++i) U[i] = V[permute[i].i-1]; // [rand()%n]; // fixed dups
  free_vec (permute);
  return U;
}

// permute in-place: scatter values into random columns [1..nc]
void permute_vec (ix_t *V, uint nc) {
  uint i, n = len(V);
  ix_t *P = rand_vec(nc);
  trim_vec (P, n);
  for (i=0; i<n; ++i) V[i].i = P[i].i;
  free_vec(P);
}

inline float powa (float x, float p) {
  if (p == 2) return x*x;
  if (p == 1) return ABS(x);
  if (p ==.5) return sqrt(ABS(x));
  if (x == 0) return 0;
  return exp (p * log (ABS(x)));
}

inline float rnd() { return (float) random() / RAND_MAX; }

ix_t *const_vec (uint n, float x) {
  ix_t *vec = new_vec (n, sizeof(ix_t)), *v, *end = vec + n;
  for (v = vec; v < end; ++v) { v->i = v - vec + 1; v->x = x; }
  return vec;
}

ix_t *rand_vec (uint n) {
  ix_t *vec = new_vec (n, sizeof(ix_t)), *v, *end = vec + n;
  for (v = vec; v < end; ++v) {
    v->i = v - vec + 1;
    v->x = rnd();
  }
  return vec;
}

ix_t *rand_vec_uni (uint n, float lo, float hi) { // uniform over [lo..hi]
  ix_t *vec = rand_vec (n), *v, *end = vec + len(vec);
  for (v = vec; v < end; ++v) v->x = lo + (hi-lo) * v->x;
  return vec;
}

ix_t *rand_vec_std (uint n) { // Normal distribution (Gaussian) N(0,1)
  ix_t *vec = rand_vec (n), *v, *end = vec + len(vec);
  for (v = vec; v < end; ++v) v->x = ppndf(v->x);
  return vec;
}

ix_t *rand_vec_exp (uint n) { // exponential distribution
  ix_t *vec = rand_vec (n), *v, *end = vec + len(vec);
  for (v = vec; v < end; ++v) v->x = - log (v->x);
  return vec;
}

ix_t *rand_vec_log (uint n) { // logistic distribution
  ix_t *vec = rand_vec (n), *v, *end = vec + len(vec);
  for (v = vec; v < end; ++v) v->x = - log (1/v->x - 1);
  return vec;
}

// uniformly over a unit sphere
ix_t *rand_vec_sphere (uint n) {
  ix_t *vec = rand_vec_std (n);
  vec_x_num (vec, '/', sqrt(sum2(vec)));
  return vec;
}

// uniformly over a multinomial simplex
ix_t *rand_vec_simplex (uint n) {
  ix_t *vec = rand_vec_exp (n);
  vec_x_num (vec, '/', sum(vec));
  return vec;
}

ix_t *rand_vec_sparse (uint n, uint k) {
  ix_t *vec = new_vec (0, sizeof(ix_t)), new = {0,0};
  while (len(vec) < k) {
    uint r = random();
    new.i = 1 + r % n;
    new.x = (float)r / RAND_MAX;
    vec = append_vec (vec, &new);
  }
  sort_uniq_vec (vec);
  return vec;
}

void rand_mtx (coll_t *M, uint nr, uint nc) {
  uint r;
  for (r = 1; r <= nr; ++r) {
    ix_t *vec = rand_vec (nc);
    put_vec (M, r, vec);
    free_vec (vec);
  }
}

void rand_mtx_sparse (coll_t *M, uint nr, uint nc, uint k) {
  uint r;
  for (r = 1; r <= nr; ++r) {
    ix_t *vec = rand_vec_sparse (nc, k);
    put_vec (M, r, vec);
    free_vec (vec);
  }
}


ix_t *num2bitvec (ulong num) {
  ix_t *vec = rand_vec (64); uint i;
  for (i = 0; i < 64; ++i) vec[i].x = (num >> i) & 1;
  chop_vec (vec);
  return vec;
}

ulong bitvec2num (ix_t *vec) {
  ulong num = 0, i;
  for (i = 0; i < 64; ++i) num = (num << 1) | (vec[i].x > 0);
  return num;
}

ix_t *bits2codes (ix_t *vec, uint ncodes) {
  uint bits = len(vec) / ncodes, c, maxcode = (1 << bits);
  ix_t *code = new_vec (ncodes, sizeof(ix_t)), *v;
  for (v = vec; v < vec + len(vec); ++v) {
    uint id = v->i - 1, bit = id % bits, c = id / bits;
    if (v->x > 0) code[c].i |= (1 << bit); // set the bit to 1
    //fprintf (stderr, " %u %f %u %u %u\n", id, v->x, c, bit, code[c].i);
  }
  //fprintf (stderr, "\n");
  for (c = 0; c < ncodes; ++c) {
    code[c].i += c * maxcode + 1; // shift each code into unique range
    code[c].x = 1;
    //fprintf (stderr, " %d", code[c].i);
  }
  //fprintf (stderr, "\n");
  //sort_uniq_vec (fps); chop_vec (fps);
  return code;
}

//inline float invNcdf (float p) {
//  float q = 2 * p - 1;
//  return sqrt (- log (1 - q*q) * PI / 2); // this is only half (for <0 or >0)
//}

double logit (double x) { return - log (1/x - 1); }

double ident (double x) { return x; }

ix_t *simhash (ix_t *vec, uint n, char *distr) {
  //double (*F) (double) = (distr == 'N') ? ppndf : (distr == 'L') ? logit : ident;
  double M = 4294967295; // largest possible value
  ix_t *B = const_vec (n, 0), *b;
  ix_t *v, *end = vec + len(vec);
  for (v = vec; v < end; ++v) {
    uint R = v->i;
    //srandom(R);
    switch (distr[0]) {
    case 'N': for (b=B; b<B+n; ++b) b->x += v->x * ppndf ((R = murmur3uint(R)) / M); break;
    case 'L': for (b=B; b<B+n; ++b) b->x += v->x * logit ((R = murmur3uint(R)) / M); break;
    case 'B': for (b=B; b<B+n; ++b) b->x += v->x * SGN ((int)(R = murmur3uint(R)));  break;
      //default : for (b=B; b<B+n; ++b) b->x += (random() - RAND_MAX/2);      break;
    default : for (b=B; b<B+n; ++b) b->x += v->x * (int) (R = murmur3uint(R));              break;
    }
    //for (b = B; b < BB; ++b) b->x += v->x * (R = murmur3uint(R));
    //X[b].x += v->x * ppndf ((R = murmur3uint (R)) / RAND_MAX);
    //X[b].x += v->x * rnd();
  }
  return B;
  //ix_t *fps = vec2fps (B, L);
  //free_vec (B);
  //return fps;
}

void sort_vecs (coll_t *c, char *prm) {
  char *aggr = getprmp(prm,"aggr:","1st");
  //printf ("%s, %s\n", prm, aggr);
  uint i, n = nvecs(c);
  fprintf (stderr, "[%.0fs] sorting %d vecs of %s\n", vtime(), n, c->path);
  for (i = 1; i <= n; ++i) {
    ix_t *vec = get_vec (c, i);
    sort_vec (vec, cmp_ix_i); // rsort? no! must be stable for aggr
    aggr_vec (vec, aggr[0]); // aggregate duplicate cols in the row
    chop_vec (vec);
    put_vec (c, i, vec);
    free_vec (vec);
    show_progress (i, n, "rows");
  }
}

void scan_mtx (coll_t *rows, coll_t *cols, hash_t *rh, hash_t *ch, char *prm) {
  while (1) {
    uint size = MIN(4294967295,MAP_SIZE/sizeof(jix_t));
    jix_t *buf = scan_jix (stdin, size/8, rh, ch);
    if (!len(buf)) {free_vec (buf); break;}
    if (rows) append_jix (rows, buf);
    if (cols) transpose_jix (buf);
    if (cols) append_jix (cols, buf);
    fprintf (stderr, "[%.0fs] added %d cells -> %d rows, %d cols\n",
	     vtime(), len(buf), num_rows(rows), num_cols(rows));
    free_vec (buf);
  }
  if (strstr(prm,"nosort")) return;
  if (rows) sort_vecs (rows, prm);
  if (cols) sort_vecs (cols, prm);
}

void print_mtx (coll_t *rows, hash_t *rh, hash_t *ch, char *how) {
  uint id, txt = !strcmp(how,"txt"), svm = !strcmp(how,"svm");
  for (id = 1; id <= nvecs(rows); ++id) {
    char *rid = id2str(rh,id);
    ix_t *row = get_vec (rows, id);
    if      (txt) print_vec_txt (row, ch, rid, 0);
    else if (svm) print_vec_svm (row, ch, rid, NULL);
    else          print_vec_rcv (row, ch, rid, NULL);
    free (rid);
    free_vec(row);
  }
}

void print_binary_2(uint n, uint sz) {
  for (; sz--; n>>=1) putchar(((n&1)?'+':'-'));
}

void print_binary(uint n, uint sz) {
  uint mask = 1<<(sz-1);
  for (; sz--; n<<=1) putchar(((n&mask)?'+':'-'));
}

void print_span(float x) {
  it_t span = unpack_span(x);
  printf("%d:%d", span.i, span.t);
}

void print_vec_rcv (ix_t *vec, hash_t *ids, char *vec_id, char *fmt) {
  if (!fmt) fmt = "%.6f";
  uint binary = (*fmt == 'b') ? atoi(fmt+1) : 0;
  uint pos = !strncmp(fmt,"pos",3);
  ix_t *v = vec, *end = vec + len(vec);
  for (v = vec; v < end; ++v) {
    char *id = id2str (ids, v->i);
    fputs(vec_id,stdout); putchar('\t');
    fputs(id,stdout);     putchar('\t'); //printf ("%s\t%s\t", vec_id, id);
    if   (binary) print_binary ((uint)(v->x), binary);
    else if (pos) print_span (v->x);
    else          printf (fmt, v->x);
    putchar('\n');
    free (id);
  }
}

void print_vec_json (ix_t *vec, hash_t *ids, char *vec_id, char *fmt) {
  if (!fmt) fmt = "%g";
  ix_t *v = vec-1, *end = vec + len(vec);
  printf ("{ \"id\":\"%s\"", vec_id);
  while (++v < end) {
    if (ids) printf (", \"%s\":", id2key(ids,v->i));
    else     printf (", \"%u\":", v->i);
    printf (fmt, v->x);
  }
  printf (" }\n");
}

void print_vec_svm (ix_t *vec, hash_t *ids, char *vec_id, char *fmt) {
  if (!fmt) fmt = "%.4f";
  ix_t *v = vec-1, *end = vec + len(vec);
  printf ("%s", vec_id);
  while (++v < end) {
    if (ids) printf (" %s:", id2key(ids,v->i));
    else     printf (" %u:", v->i);
    printf (fmt, v->x);
  }
  printf ("\n");
}

ix_t *parse_vec_svm (char *str, char **id, hash_t *ids) {
  if (id) *id = strdup (next_token (&str," \t"));
  ix_t *vec = new_vec (0, sizeof(ix_t)), new = {0,0}; char *key;
  while ((key = next_token (&str," \t"))) {
    char *val = strchr(key,':');
    if (!val) continue;
    *val++ = 0; // null-terminate key + advance to value
    new.i = ids ? key2id(ids,key) : (uint) atol(key);
    new.x = atof(val);
    if (new.i && new.x) vec = append_vec (vec, &new);
  }
  int bytes;
  while (2 == sscanf (str, " %d:%f%n", &(new.i), &(new.x), &bytes)) {
    if (new.i && new.x) vec = append_vec (vec, &new);
    str += bytes;
  }
  sort_uniq_vec (vec);
  return vec;
}

void print_hdr_csv (hash_t *H, uint ncols) {
  uint i;
  for (i = 1; i <= ncols; ++i) printf("\t%s", id2key(H,i));
  putchar('\n');
}

void print_vec_csv (ix_t *vec, uint ncols, char *rid, char *fmt) {
  if (!fmt || !*fmt) fmt = "%.2f";
  if (!ncols && len(vec)) ncols = vec[len(vec)-1].i;
  if (rid && *rid) printf("%s",rid);
  float *full = vec2full (vec, ncols, 0), *end = full+len(full), *f;
  for (f = full+1; f < end; ++f) { putchar('\t'); printf (fmt, *f); }
  printf ("\n");
  free_vec (full);
}

ix_t *parse_vec_csv (char *str, char **id) {
  int bytes;
  if (id) *id = strdup (next_token (&str," \t"));
  ix_t *vec = new_vec (0, sizeof(ix_t)), new = {0,0};
  //printf ("\n%s ->", str);
  while (1 == sscanf(str," %f%n", &(new.x), &bytes)) {
    new.i++;
    //printf (" %f", new.x);
    //if (new.x) vec = append_vec (vec, &new);
    vec = append_vec (vec, &new);
    str += bytes;
  }
  //printf ("%d\n", len(vec));
  return vec;
}

void print_vec_txt (ix_t *vec, hash_t *ids, char *vec_id, int xml) {
  //if (!len(vec)) return;
  //float factor = 1; // 100 / (max(vec)->x);
  if (xml) printf ("<DOC id=\"%s\">", vec_id);
  else printf ("%s", vec_id);
  ix_t *v = vec, *end = vec + len(vec);
  for (v = vec; v < end; ++v) {
    char *id = id2str (ids, v->i);
    printf (" %s", id);
    free (id);
    //for (n = 0; n < factor * v->x; ++n) printf (" %s", id);
  }
  printf (xml ? "</DOC>\n" : "\n");
}

ix_t *parse_as_ngrams (char *str, hash_t *ids, int ngramsz, char *positional) {
  ix_t *vec = new_vec (0, sizeof(ix_t)); int n;
  sjk_t *tokens = good_tokens (str);
  stop_tokens (tokens);
  for (n = 1; n <= ngramsz; ++n) {
    sjk_t *T = good_ngrams (tokens, n), *t;
    for (t=T; t < T+len(T); ++t) {
      float x = positional ? pack_span(t->j,t->k) : 1;
      uint id = key2id (ids, t->s);
      ix_t el = {id, x};
      if (el.i) vec = append_vec (vec, &el);
    }
    free_tokens (T);
  }
  free_tokens (tokens);
  if (positional) sort_vec (vec, cmp_ix_x);
  else sort_uniq_vec (vec);
  return vec;
}

ix_t *parse_vec_txt (char *str, char **id, hash_t *ids, char *prm) { // thread-unsafe: stop_toks
  char *positional = strstr (prm, "position");
  char *stemmer = getprmp (prm, "stem=", "L");
  uint gram = getprm (prm,"gram=",0), gram_hi = getprm (prm,":",gram);
  uint step = getprm (prm,"char=",1);
  uint toklen = getprm (prm, "toklen=", 50);
  int ngramsz = getprm (prm,"ngramsz=",0);
  char *stop = strstr (prm, "stop");
  char *nowb = strstr (prm, "nowb");
  char *buf = 0, *ws = strstr(prm,"tokw") ? " \t\r\n" : NULL;
  if (id) *id = strdup (next_token (&str, " \t"));
  if (ngramsz) return parse_as_ngrams (str, ids, ngramsz, positional);
  if (nowb) squeeze (str, NULL, NULL); // squeeze out punctuation and spaces
  if (gram) {
    buf = malloc (1<<26);
    csub (str, ws, '_'); // replace all runs of punctuation with underscore
    cgrams (str, gram, gram_hi, step, buf, 1<<26); // character n-grams
    str = buf; ws = " ";
  }
  char **toks = str2toks (str, ws, toklen); // strdup (position lost)
  if (stemmer) stem_toks (toks, stemmer); // free & strdup
  if (stop)    stop_toks (toks); // free
  char **pairs = strstr (prm,"w=") ? toks2pairs (toks,prm) : NULL;
  ix_t *vec = toks2vec (pairs?pairs:toks, ids), *v;
  if (positional) // sequential token positions
    for (v = vec; v < vec + len(vec); ++v) v->x = v - vec + 1;
  else
    sort_uniq_vec (vec);
  //if (stemmer) free(stemmer);
  if (buf) free(buf);
  free_toks (pairs);
  free_toks (toks);
  return vec;
}

ix_t *parse_vec_xml (char *str, char **id, hash_t *ids, char *prm) {
  if (id) *id = get_xml_docid (str);
  erase_between (str, "<DOCID>", "</DOCID>", ' ');
  no_xml_tags (str);
  //erase_between (str, "<", ">", ' '); // remove all tags
  return parse_vec_txt (str, NULL, ids, prm); // normal text, but no docid
}

uint is_dense (ix_t *V) { uint n=len(V); return V && n && V[n-1].i == n; }

uint last_id  (ix_t *V) { uint n=len(V); return (V && n) ? V[n-1].i : 0; }

uint num_rows (coll_t *c) { return (!c) ? 0 : c->rdim ? c->rdim : (c->rdim = nvecs(c)); }

uint num_cols (coll_t *c) {
  if (!c) return 0;
  if (c->cdim) return c->cdim;
  uint id, N = 0, nr = num_rows (c);
  for (id = 1; id <= nr; ++id) {
    void *vec = get_vec_ro (c, id);
    uint j = last_id (vec);
    if (j > N) N = j;
  }
  return (c->cdim = N);
}

uint *row_ids(coll_t *rows) { // list of non-empty row ids
  uint id = 0, nr = num_rows (rows);
  uint *X = new_vec (0, sizeof(uint));
  for (id = 1; id <= nr; ++id)
    if (has_vec(rows,id)) X = append_vec (X,&id);
  return X;
}

uint *len_rows (coll_t *rows) {
  uint id, nr = num_rows (rows);
  uint *X = new_vec (nr+1, sizeof(uint));
  for (id = 1; id <= nr; ++id) {
    ix_t *row = get_vec_ro (rows, id);
    X[id] = len(row);
  }
  return X;
}

uint *len_cols (coll_t *rows) {
  uint id, nr = num_rows(rows), nc = num_cols (rows);
  uint *X = new_vec (nc+1, sizeof(uint));
  for (id = 1; id <= nr; ++id) {
    ix_t *row = get_vec_ro (rows,id), *end = row + len(row), *r = row-1;
    assert (end == row || end[-1].i <= nc); // check out-of-range [DEBUG]
    while (++r < end) X[r->i] += 1;
    if (0 == id%100) show_progress (id, nr, "rows (len_cols)");
  }
  return X;
}

float *sum_rows (coll_t *rows, float p) {
  uint id = 0, nr = num_rows(rows);
  float *X = new_vec (nr+1, sizeof(float));
  while (++id <= nr) {
    ix_t *vec = get_vec_ro (rows,id);
    X[id] = sump (p, vec);
    if (0==id%100) show_progress (id,nr,"rows (row:sum)");
  }
  return X;
}

float *norm_rows (coll_t *rows, float p, float root) {
  uint id = 0, nr = num_rows(rows);
  float *X = new_vec (nr+1, sizeof(float));
  while (++id <= nr) {
    ix_t *vec = get_vec_ro (rows,id);
    X[id] = norm (p, root, vec);
  }
  return X;
}

float *sum_cols (coll_t *rows, float p) {
  uint id, nr = num_rows(rows), nc = num_cols(rows);
  float *X = new_vec (nc+1, sizeof(float));
  for (id = 1; id <= nr; ++id) {
    ix_t *row = get_vec_ro (rows, id), *end = row + len(row), *r = row-1;
    if      (p == 1) while (++r<end) X[r->i] += r->x;
    else if (p == 2) while (++r<end) X[r->i] += r->x * r->x;
    else if (p == 0) while (++r<end) X[r->i] += (r->x != 0);
    else if (p ==.5) while (++r<end) X[r->i] += sqrt(r->x);
    else             while (++r<end) X[r->i] += powf (r->x, p);
    if (0==id%100) show_progress (id,nr,"rows (col:sum)");
  }
  return X;
}

float *norm_cols (coll_t *rows, float p, float root) {
  uint id, nr = num_rows(rows), nc = num_cols(rows);
  float *X = new_vec (nc+1, sizeof(float));
  for (id = 1; id <= nr; ++id) {
    ix_t *row = get_vec_ro (rows, id), *end = row + len(row), *r = row-1;
    if      (p == 1) while (++r<end) X[r->i] += ABS(r->x);
    else if (p == 2) while (++r<end) X[r->i] += r->x * r->x;
    else if (p == 0) while (++r<end) X[r->i] += (r->x != 0);
    else if (p ==.5) while (++r<end) X[r->i] += sqrt (ABS(r->x));
    else             while (++r<end) X[r->i] += powa (r->x, p);
  }
  if (root) for (id = 1; id <= nc; ++id) X[id] = powa (X[id], root);
  return X;
}

float *cols_avg (coll_t *rows, float p) {
  uint i, nc = num_cols(rows);
  uint  *df = len_cols (rows);
  float *cf = sum_cols (rows,p);
  for (i=1; i<=nc; ++i) if (df[i]) cf[i] /= df[i];
  free_vec (df);
  return cf;
}

double sum_sums (coll_t *rows, float p) {
  uint id, nr = num_rows(rows);
  double X = 0;
  for (id = 1; id <= nr; ++id) {
    ix_t *vec = get_vec_ro (rows,id);
    X += sump (p, vec);
  }
  return X;
}

xy_t mrange (coll_t *m) {
  uint i, n = num_rows(m);
  xy_t range = {Infinity, -Infinity};
  for (i = 1; i <= n; ++i) {
    ix_t *row = get_vec_ro (m,i), *r, *end = row+len(row);
    for (r = row; r < end; ++r) {
      if (r->x < range.x) range.x = r->x;
      if (r->x > range.y) range.y = r->x; }
  }
  return range;
}

double max_maxs (coll_t *c) {
  uint id, nr = num_rows(c);
  double X = -Infinity;
  for (id = 1; id <= nr; ++id) {
    ix_t *vec = get_vec_ro (c,id);
    double x = max(vec)->x;
    X = MIN(X,x);
  }
  return X;
}

jix_t *max_rows (coll_t *rows) {
  uint id, nr = num_rows(rows);
  jix_t *M = new_vec (0, sizeof(jix_t));
  for (id = 1; id <= nr; ++id) {
    ix_t *row = get_vec_ro (rows, id);
    if (len(row)) {
      ix_t *r = max(row);
      jix_t new = {id, r->i, r->x};
      M = append_vec (M, &new);
    }
  }
  return M;
}

jix_t *min_rows (coll_t *rows) {
  uint id, nr = num_rows(rows);
  jix_t *M = new_vec (0, sizeof(jix_t));
  for (id = 1; id <= nr; ++id) {
    ix_t *row = get_vec_ro (rows, id);
    if (len(row)) {
      ix_t *r = min(row);
      jix_t new = {id, r->i, r->x};
      M = append_vec (M, &new);
    }
  }
  return M;
}

jix_t *max_cols (coll_t *rows) {
  uint id, nr = num_rows(rows), nc = num_cols(rows);
  jix_t *M = new_vec (nc, sizeof(jix_t)), *m;
  for (m = M; m < M+nc; ++m) { m->x = -Infinity; m->i = m-M+1; }
  for (id = 1; id <= nr; ++id) {
    ix_t *row = get_vec_ro (rows, id), *end = row + len(row), *r;
    for (r = row; r < end; ++r) {
      m = M + r->i - 1;
      if (r->x > m->x) { m->j = id; m->x = r->x; }
    }
  }
  return M;
}

jix_t *min_cols (coll_t *rows) {
  uint id, nr = num_rows(rows), nc = num_cols(rows);
  jix_t *M = new_vec (nc, sizeof(jix_t)), *m;
  for (m = M; m < M+nc; ++m) { m->x = Infinity; m->i = m-M+1; }
  for (id = 1; id <= nr; ++id) {
    ix_t *row = get_vec_ro (rows, id), *end = row + len(row), *r;
    for (r = row; r < end; ++r) {
      m = M + r->i - 1;
      if (r->x < m->x) { m->j = id; m->x = r->x; }
    }
  }
  return M;
}

void m_range (coll_t *m, int top, float *_min, float *_max) {
  float min = Infinity, max = -Infinity;
  uint i, n = num_rows(m);
  for (i = 1; i <= n; ++i) {
    ix_t *row = get_vec (m,i), *r;
    if (top) trim_vec (row, top);
    for (r = row; r < row+len(row); ++r) {
      if (r->x < min) min = r->x;
      if (r->x > max) max = r->x;
    }
    //printf ("%d range: %f ... %f\n", len(row), min, max);
    free_vec (row);
  }
  *_min = min; *_max = max;
}

void free_stats (stats_t *s) {
  if (!s) return;
  if (s->df) free_vec (s->df);
  if (s->cf) free_vec (s->cf);
  if (s->s2) free_vec (s->s2);
  memset (s, 0, sizeof(stats_t));
  free_vec (s);
}

stats_t *blank_stats (int df, int cf, int s2, uint n) {
  stats_t *s    = new_vec (1, sizeof(stats_t));
  if (df) s->df = new_vec (n, sizeof(ulong));
  if (cf) s->cf = new_vec (n, sizeof(double));
  if (s2) s->s2 = new_vec (n, sizeof(double));
  return s;
}

static inline void grow_stats (stats_t *s, uint id) {
  if (id < len(s->df)) return; // big enough
  s->nwords = id;
  if (s->df) s->df = resize_vec (s->df, id+1);
  if (s->cf) s->cf = resize_vec (s->cf, id+1);
  if (s->s2) s->s2 = resize_vec (s->s2, id+1);
}

void update_stats_from_vec (stats_t *s, ix_t *vec) {
  if (!vec || !len(vec)) return;
  ix_t *v = vec-1, *end = vec + len(vec);
  grow_stats (s, end[-1].i); // make sure enough space
  while (++v < end) {
    if (s->df) s->df [v->i] += (v->x != 0);
    if (s->cf) s->cf [v->i] += v->x;
    if (s->s2) s->s2 [v->i] += v->x * v->x;
    s->nposts += v->x;
  }
  s->ndocs += 1;
}

stats_t *coll_stats (coll_t *c) {
  uint r, nr = num_rows (c);
  stats_t *s = blank_stats (1,1,1,0);
  for (r = 1; r <= nr; ++r) {
    ix_t *vec = get_vec_ro (c, r);
    update_stats_from_vec (s, vec);
    show_progress (r, nr, " stats");
  }
  //s->ndocs = num_rows (c);
  //s->nwords = num_cols (c);
  //s->df = len_cols (c);
  //s->cf = sum_cols (c, 1);
  //s->s2 = sum_cols (c, 2);
  //s->nposts = sumf (s->cf);
  return s;
}

void update_stats_from_file (stats_t *s, hash_t *dict, char *file) {
  char line[1000], word[1000];
  uint id, df; float cf;
  FILE *in = safe_fopen (file, "r");
  while (fgets (line, 999, in)) {
    if (*line == '#') continue; // skip comments
    if (3 != sscanf (line, "%d %f %s", &df, &cf, word)) continue; // can't parse
    id = key2id (dict, word);
    if (id) {
      grow_stats (s, id);
      s->df[id] += df;
      s->cf[id] += cf;
    }
    s->nposts += cf;
    if (df > s->ndocs) s->ndocs = df;
  }
  fclose(in);
}

void dump_stats (stats_t *s, hash_t *dict, char *prm) {
  ulong  minDF = getprm(prm,"df>",0), maxDF = getprm(prm,"df<",0);
  double minCF = getprm(prm,"cf>",0), maxCF = getprm(prm,"cf<",0);
  uint w = 0, nw = s->nwords;
  //FILE *out = safe_fopen (file, "w");
  fprintf (stderr, "#\t%ld\t%ld\t%.0f\tdocs/words/posts\n", s->ndocs, s->nwords, s->nposts);
  while (++w <= nw) {
    ulong df = s->df[w]; double cf = s->cf[w];
    if ((df <= minDF) || (maxDF && df >= maxDF)) continue;
    if ((cf <= minCF) || (maxCF && cf >= maxCF)) continue;
    char *key = id2str(dict,w);
    //printf ("%15s %5ld %10.2f\n", key, s->df[w], s->cf[w]);
    printf ("%ld\t%.2f\t%s\n", s->df[w], s->cf[w], key);
    free (key);
  }
  //fclose (out);
}

void save_stats (stats_t *s, char *path) {
  char x[1000];
  write_vec (s,     fmt(x,"%s.stats",path));
  write_vec (s->df, fmt(x,"%s.df",path));
  write_vec (s->cf, fmt(x,"%s.cf",path));
}

stats_t *open_stats (char *path) {
  char x[1000];
  stats_t *s = read_vec (fmt(x,"%s.stats",path));
  s->df      = read_vec (fmt(x,"%s.df",path));
  s->cf      = read_vec (fmt(x,"%s.cf",path));
  s->s2 = NULL;
  return s;
}

stats_t *open_stats_if_exists (char *path) {
  return file_exists ("%s.stats", path) ? open_stats (path) : NULL;
}

// extract assignment as jix, sort, push into rows
void disjoin_rows_append (coll_t *rows) {
  uint id, nr = num_rows (rows);
  jix_t *M = max_cols (rows); // M = [{max_row, col, max_val}]
  chop_jix (M);
  sort_vec (M, cmp_jix);
  for (id = 1; id <= nr; ++id) del_vec (rows, id);
  append_jix (rows, M);
  free_vec (M);
}

// transpose rows, trim to 1 max element, transpose back
void disjoin_rows_transpose (coll_t *rows) {
  coll_t *cols = open_coll (NULL, NULL);
  transpose_mtx (rows, cols);
  uint r, c, nr = num_rows(rows), nc = num_rows(cols);
  for (c = 1; c <= nc; ++c) {
    ix_t *col = get_vec (cols, c), *best = max(col);
    *col = *best;
    len(col) = 1;
    put_vec (cols, c, col);
    free_vec (col);
  }
  for (r = 1; r <= nr; ++r) del_vec (rows, r);
  transpose_mtx (cols, rows);
  free_coll (cols);
}

// extract assignment as jix, filter each row
void disjoin_rows_filter (coll_t *rows) {
  uint j, nr = num_rows (rows);
  jix_t *M = max_cols (rows); // M[col-1] = {max_row, col, max_val}
  for (j = 1; j <= nr; ++j) {
    ix_t *R = get_vec (rows, j), *r;
    for (r = R; r < R+len(R); ++r) {
      jix_t *assigned = M + r->i - 1; // assignment of column r->i
      if (assigned->j != j) r->x = 0; // not assigned to this row
      if (assigned->i != r->i) assert (0 && "disjoin_rows_chop > max_cols error");
    }
    chop_vec (R);
    put_vec (rows, j, R);
    free_vec (R);
  }
  free_vec (M);
}

// make all rows disjoint (assign column to best row)
void disjoin_rows (coll_t *rows) { disjoin_rows_filter(rows); }

#define vget(v,i,x) (((i) < len(v)) ? ((v)[i]) : (x))

void weigh_vec_clarity (ix_t *vec, stats_t *s) {
  ix_t *v, *end = vec + len(vec);
  double dl = sum (vec);
  for (v = vec; v < end; ++v) {
    double Pvd = v->x / dl;
    double Pv = vget (s->cf, v->i, 1) / s->nposts;
    v->x = Pvd * log (Pvd / Pv);
  }
}

ix_t *doc2lm (ix_t *doc, double *cf, double mu, double lambda) { // thread-unsafe: static
  static ix_t *BG = NULL;
  if (!BG) { BG = double2vec (cf); vec_x_num (BG, '/', sum(BG)); }
  double dl = sum(doc), a = lambda / (dl?dl:1);
  ix_t *LM = (mu ? vec_add_vec (1, doc,  mu, BG)
	      :    vec_add_vec (a, doc, 1-a, BG));
  vec_x_num (LM, '/', sum(LM));
  return LM;
}

void weigh_vec_lmj (ix_t *vec, stats_t *s) {
  ix_t *v, *end = vec + len(vec);
  float dl = sum (vec), odds = s->b / (1 - s->b);
  for (v = vec; v < end; ++v) {
    double Pvd = v->x / dl;
    double Pv = vget (s->cf, v->i, 1) / s->nposts;
    v->x = log (1 + odds * Pvd / Pv);
  }
}

void weigh_vec_lmd (ix_t *vec, stats_t *s) {
  ix_t *v, *end = vec + len(vec);
  for (v = vec; v < end; ++v) {
    double Pvd = v->x / s->k; // yes, this is correct
    double Pv = vget (s->cf, v->i, 1) / s->nposts;
    v->x = log (1 + Pvd / Pv);
  }
}

void weigh_vec_idf (ix_t *vec, stats_t *s) {
  ix_t *v, *end = vec + len(vec);
  for (v = vec; v < end; ++v) {
    ulong df = vget (s->df, v->i, 1), nd = s->ndocs;
    double idf = log ((0.5 + nd) / df) / log (1.0 + nd);
    v->x = v->x * idf;
  }
}

void weigh_vec_inq (ix_t *vec, stats_t *s) {
  ix_t *v, *end = vec + len(vec);
  double dl = sum (vec), avgdl = s->nposts / s->ndocs;
  for (v = vec; v < end; ++v) {
    ulong df = vget (s->df, v->i, 1), nd = s->ndocs;
    double idf = log ((0.5 + nd) / df) / log (1.0 + nd);
    double ntf = v->x / (v->x + 0.5 + 1.5 * dl / avgdl);
    v->x = ntf * idf; // 0.4 + 0.6 * ntf * idf ... problems
  }
}

void weigh_vec_ntf (ix_t *vec, stats_t *s) {
  ix_t *v, *end = vec + len(vec);
  double dl = sum (vec), avgdl = s->nposts / s->ndocs;
  double k = s->k ? s->k : 2.0, b = s->b ? s->b : 0.75;
  for (v = vec; v < end; ++v)
    v->x = v->x * (1 + k) / (v->x + k * ((1-b) + b * dl / avgdl));
}

void weigh_vec_bm25 (ix_t *vec, stats_t *s) {
  ix_t *v, *end = vec + len(vec);
  double dl = sum (vec), avgdl = s->nposts / s->ndocs;
  double k = s->k ? s->k : 2.0, b = s->b ? s->b : 0.75;
  //printf ("b=%f k=%f nd=%d\n", b, k, s->ndocs);
  for (v = vec; v < end; ++v) {
    ulong df = vget (s->df, v->i, 1), nd = s->ndocs;
    //float idf = log ((nd - df + 0.5) / (df + 0.5)); // negative if df < nd/2
    double idf = log ((0.5 + nd) / df) / log (1.0 + nd); // use inquery idf
    double ntf = v->x * (1 + k) / (v->x + k * ((1-b) + b * dl / avgdl));
    v->x = ntf * idf; // use MAX(0,idf) with non-inquery idf
  }
}

// make the values have unit variance from the given mean
void weigh_vec_unit_variance (ix_t *V, float mean) {
  double SSE = 0; // sum of squared deviations from mean
  ix_t *v = V-1, *end = V + len(V);
  for (v = V; v < end; ++v) SSE += (v->x - mean) * (v->x - mean);
  SSE = sqrt(SSE/(len(V) - 1));
  for (v = V; v < end; ++v) v->x /= SSE;
}

void weigh_vec_std (ix_t *vec, stats_t *s) {
  //static uint dump = 1;
  ulong *DF = s->df;
  double *S1 = s->cf, *S2 = s->s2;
  ix_t *v, *end = vec + len(vec);
  for (v = vec; v < end; ++v) {
    float N = vget (DF, v->i, 1);
    float mean = vget (S1, v->i, 0) / N;
    float mean2 = vget (S2, v->i, 0) / N;
    float std = sqrt (mean2 - mean * mean);
    float new = (v->x - mean) / (std ? std : 1);
    if (isnan(new)) {
      fprintf(stderr,"STD: [%d] %.4f - %.4f / %.4f (%.4f) [%.0f]\n",
	      v->i, v->x, mean, std, mean2, N);
      assert(0);
    }
    v->x = new;
    //if (dump) fprintf (stderr, "%d     %.10f     %.10f\n", v->i, mean, std);
  }
  //dump = 0;
}

void weigh_vec_laplacian (ix_t *vec, stats_t *s) {
  double rowsum = sum(vec), *colsum = s->cf;
  ix_t *v = vec-1, *end = vec + len(vec);
  while (++v < end) v->x /= sqrt (rowsum * vget (colsum,v->i,1));
}

void weigh_vec_range01 (ix_t *vec) {
  float m = min(vec)->x, M = max(vec)->x;
  vec_x_num (vec, '-', m);
  vec_x_num (vec, '/', M-m);
}

void weigh_vec_rnd (ix_t *vec) {
  ix_t *v = vec-1, *end = vec + len(vec);
  while (++v < end) v->x = rnd();
}

void weigh_vec_ranks (ix_t *vec) { // rank
  sort_vec (vec, cmp_ix_X);
  ix_t *v = vec-1, *end = vec + len(vec);
  while (++v < end) v->x = (v-vec+1);
  sort_vec (vec, cmp_ix_i);
}

void weigh_vec_cdf (ix_t *vec) { // values -> cumulative distribution function
  sort_vec (vec, cmp_ix_x);
  ix_t *v = vec-1, *end = vec + len(vec);
  while (++v < end) v->x = (v-vec+1.0) / (end-vec);
  sort_vec (vec, cmp_ix_i);
}

//void filter_vec (ix_t *vec, uint *keep) {
//  ix_t *u, *v, *end = vec+len(vec);
//  for (v = vec; v < end; ++v) if (keep[v->i]) *u++ = *v;
//  resize_vec (vec, u - vec);
//}

void weigh_invl_lmd (ix_t *invl, stats_t *s, float mu) {
  ix_t *d, *end = invl + len(invl);
  for (d = invl; d < end; ++d) {
    double Pv = vget (s->cf, d->i, 1) / s->nposts;
    d->x = log (1 + d->x / (mu * Pv));
  }
}

void zstd_outliers (ix_t *vec, stats_t *s, float out) {
  ulong *DF = s->df;
  double *S1 = s->cf, *S2 = s->s2;
  ix_t *v, *end = vec + len(vec);
  for (v = vec; v < end; ++v) {
    double N = vget (DF, v->i, 1);
    double mean = vget (S1, v->i, 0) / N;
    double mean2 = vget (S2, v->i, 0) / N;
    double stdev = sqrt (mean2 - mean * mean);
    double lo = mean - out*stdev, hi = mean + out*stdev, x=v->x;
    if (x < lo) v->x = lo;
    if (x > hi) v->x = hi;
    if (v->x != x) fprintf (stderr, "OUTLIER %d: %f -> %f\n", v->i, x, v->x);
  }
}

// {x,y} s.t. at least p of the values fall into [x,y]
xy_t cdf_interval (ix_t *X, float p) {
  if (!len(X) || p <= 0 || p > 1) return (xy_t) {0,0};
  ix_t *C = distinct_values (X, 0), *last = C+len(C)-1, *c;
  for (c = C+1; c <= last; ++c) c->i += c[-1].i; // cumulative counts
  xy_t CI = {C->x, last->x}, THR = { (1-p)/2 * last->i, (1+p)/2 * last->i};
  if (len(C) < 30) return CI;
  for (c = C; c <= last; ++c) if (c->i >= THR.x) { CI.x = c->x; break; } // [x
  for (c = last; c >= C; --c) if (c->i <= THR.y) { CI.y = c->x; break; } // ??
  free_vec (C);
  return CI;
}

// {x,y} s.t. at least p of the values fall into [x,y]
xy_t median_interval (ix_t *_X, float p) {
  uint n = len(_X), drop = 1 + (1-p) * n;
  ix_t *X = copy_vec(_X);
  sort_vec(X, cmp_ix_x);
  ix_t *a = X, *z = X + n - 1;
  while (--drop > 0) {
    double mid = median(a, z-a+1);
    double da = mid - a->x, dz = z->x - mid;
    if (da < dz) --z; // z is more of an outlier
    else         ++a; // a is an outlier
  }
  xy_t CI = {a->x, z->x};
  free_vec(X);
  return CI;
}

// {x,y} s.t. at least p of the values fall into [x,y]
xy_t compact_interval (ix_t *_X, float p) {
  uint n = len(_X), drop = (1-p) * n;
  ix_t *X = copy_vec(_X);
  sort_vec(X, cmp_ix_x);
  ix_t *a = X, *z = X + n - 1;
  while (a < z && drop > 0) {
    double da = (a+1)->x - a->x; // compaction from (a) -> (a+1)
    double dz = z->x - (z-1)->x; // compaction from (z) -> (z-1)
    if (da < dz) --z; // removing z yields a more compact CI
    else         ++a; // removing a compacts more
    --drop;
  }
  xy_t CI = {a->x, z->x};
  free_vec(X);
  return CI;
}

/*
xy_t cdf_interval_q (ix_t *X, float p) { // at least p of the values are in [x,y]
  xy_t CI = {0,0};
  if (!X || !len(X) || p<=0) return CI;
  if (p>=1 || len(X) < 30)  return (xy_t) {{min(X)->x, max(X)->x};
  int top = len(X) * (1-p)/2;
  qselect (X,  top); CI.x = X[top];
  qselect (X, -top); CI.y = X[top];
  return CI;
}
*/

void keep_outliers (ix_t *X, xy_t CI) {
  fprintf (stderr, "[%f..%f] %d -> ", min(X)->x, max(X)->x, len(X));
  vec_x_range (X, '-', CI);
  fprintf (stderr, "%d [%f..%f]\n", len(X), CI.x, CI.y);
}

void drop_far_outliers (ix_t *X, xy_t CI) {
  float lo = min(X)->x, hi = max(X)->x;
  if (lo > CI.x / 2) CI.x = lo; // lo value not far
  if (hi < CI.y * 2) CI.y = hi; // hi value not far
  if (CI.x == lo && CI.y == hi) return; // nothing to do
  uint n = len(X);
  vec_x_range (X, '.', CI);
  fprintf (stderr, "[%f..%f] %d -> %d [%f..%f]\n", lo, hi, n, len(X), min(X)->x, max(X)->x);
}

void drop_outliers (ix_t *X, xy_t CI) {
  fprintf (stderr, "[%f..%f] %d -> ", min(X)->x, max(X)->x, len(X));
  vec_x_range (X, '.', CI);
  fprintf (stderr, "%d [%f..%f]\n", len(X), min(X)->x, max(X)->x);
}

void crop_outliers (ix_t *X, xy_t CI) {
  fprintf (stderr, "[%f..%f] %d -> ", min(X)->x, max(X)->x, len(X));
  vec_x_range (X, '=', CI);
  fprintf (stderr, "%d [%f..%f]\n", len(X), min(X)->x, max(X)->x);
}

// z deviations above mean score in p-confidence interval of V
float ci_value_threshold (ix_t *V, float p, float z) {
  if (!V || len(V) < 10) return -Infinity;
  xy_t s = mean_and_stdev(V, p);
  printf("[%+.4f..%+.4f] %+.4f ± %.4f ", min(V)->x, max(V)->x, s.x, s.y);
  printf("← %+.4f ← %+.4f ← %+.4f ← %+.4f ← %+.4f\n", s.x+5*s.y, s.x+4*s.y, s.x+3*s.y, s.x+2*s.y, s.x+1*s.y);
  return s.x + z * s.y;
}

// z deviations above mean delta in p-confidence interval of V
float ci_delta_threshold (ix_t *V, float p, float z) {
  int n = len(V), m = n/2 + 1;
  if (n < 1) return -Infinity;
  ix_t *D = value_deltas(V); // deltas between adjacent scores in V
  float jump = ci_value_threshold(D, p, z); // unusually large delta
  ix_t *Y = copy_vec(V);
  sort_vec(Y, cmp_ix_X);
  while (--m > 0) if (Y[m-1].x - Y[m].x > jump) break;
  float result = -Infinity;
  if (m > 0)      result = (Y[m-1].x + Y[m].x) / 2;
  else if (n > 5) result = (Y[4].x + Y[5].x) / 2;
  free_vec(D); free_vec(Y);
  return result;
}

// reweigh every vector in M, or given vector V (if M==0)
void weigh_mtx_or_vec (coll_t *M, ix_t *V, char *prm, stats_t *s) {
  char *l2p = strstr(prm,"softmax");
  float  L1 = getprm(prm,"L1=",0);
  float  L2 = getprm(prm,"L2=",0);
  float top = getprm(prm,"top=",0);
  float thr = getprm(prm,"thr=",0);
  float rbf = getprm(prm,"rbf=",0);
  float lmj = getprm(prm,"lm:j=",0); if (lmj) s->b = lmj;
  float lmd = getprm(prm,"lm:d=",0); if (lmd) s->k = lmd;
  float fix = getprm(prm,"const=",0);
  char *inq = strstr(prm,"inq");
  char *idf = strstr(prm,"idf");
  char *r01 = strstr(prm,"range01");
  char *uni = strstr(prm,"uniform");
  //xy_t R = r01 ? mrange (M) : (xy_t){0,0};
  //if (r01) fprintf(stderr,"---------> RANGE: %f ... %f\n", R.x, R.y);
  uint i=0, n = M ? num_rows (M) : 1;
  while (++i <= n) {
    ix_t *vec = M ? get_vec (M, i) : V;
    if (lmj) weigh_vec_lmj (vec, s);
    if (lmd) weigh_vec_lmd (vec, s);
    if (inq) weigh_vec_inq (vec, s);
    if (idf) weigh_vec_idf (vec, s);
    if (rbf) vec_x_num (vec, 'r', rbf);
    if (top) trim_vec (vec, top);
    if (thr) vec_x_num (vec, 'T', thr);
    if (fix) vec_x_num (vec, '=', fix);
    if (uni) vec_x_num (vec, '=', 1./len(vec));
    if (L1)  vec_x_num (vec, '/', sum(vec)/L1);
    if (L2)  vec_x_num (vec, '/', sqrt(sum2(vec)/L2));
    //if (r01) { vec_x_num (vec, '-', R.x); vec_x_num (vec, '/', R.y-R.x); }
    if (r01) weigh_vec_range01 (vec);
    if (l2p) softmax (vec);
    if (M) { put_vec (M, i, vec); free_vec (vec); }
  }
}

double log_sum_exp (ix_t *vec) {
  if (!len(vec)) return -Infinity;
  //double K = sum(vec) / len(vec); // bad idea: causes overflow
  double K = max(vec)->x, SUM = 0; // exp(max) dominates => preserve it
  ix_t *v = vec-1, *end = vec + len(vec);
  while (++v < end) SUM += exp (v->x - K);
  return log(SUM) + K;
}

void softmax (ix_t *vec) { // log2posterior
  if (!len(vec)) return;
  double K = max(vec)->x, SUM = 0;
  ix_t *v = vec-1, *end = vec + len(vec);
  while (++v < end) SUM += (v->x = exp (v->x - K));
  if (SUM) vec_x_num (vec, '/', SUM);
}

void col_softmax (coll_t *O, coll_t *M) {
  uint nr = num_rows(M), nc = num_cols(M), i;
  double *K = new_vec (nc+1, sizeof(double));
  double *S = new_vec (nc+1, sizeof(double));
  for (i=1; i<=nr; ++i) { // K [c] = max of column c across rows
    ix_t *row = get_vec_ro (M,i), *end = row+len(row), *r = row-1;
    while (++r < end) K[r->i] = MAX (r->x, K[r->i]);
  }
  for (i=1; i<=nr; ++i) { // S [c] = sum_r exp (M[r,c] - K[c])
    ix_t *row = get_vec_ro (M,i), *end = row+len(row), *r = row-1;
    while (++r < end) S[r->i] += exp (r->x - K[r->i]);
  }
  for (i=1; i<=nr; ++i) { // O[r,c] = exp(M[r,c]-K) / sum_r exp(M[r,c]-K)
    ix_t *row = get_vec (M,i), *end = row+len(row), *r = row-1;
    while (++r < end) r->x = exp (r->x - K[r->i]) / S[r->i];
    put_vec (O,i,row);
    free_vec (row);
  }
  free_vec (K); free_vec (S);
}

void split_mtx (coll_t *M, float p, coll_t *A, coll_t *B) {
  uint i; empty_coll(A); empty_coll(B);
  for (i = 1; i <= num_rows (M); ++i) {
    ix_t *a = new_vec (0, sizeof(ix_t));
    ix_t *b = new_vec (0, sizeof(ix_t));
    ix_t *V = get_vec_ro (M,i), *v;
    for (v = V; v < V+len(V); ++v)
      if (p < rnd()) a = append_vec (a, v);
      else           b = append_vec (b, v);
    put_vec (A,i,a); free_vec (a);
    put_vec (B,i,b); free_vec (b);
  }
}

/*

void prealloc_mtx (coll_t *mtx, uint *count, uint zero) {
  uint id;
  for (id = 1; id < len(count); ++id) {
    ix_t *vec = mmap_vec (mtx, id, count[id], sizeof(ix_t));
    if (zero) len(vec) = 0;
    //if (zero) resize_vec (vec, 0);
    //free_vec (vec);
  }
}

void assert_counts (coll_t *mtx, uint *count) {
  uint id;
  for (id = 1; id <= nvecs(mtx); ++id) {
    ix_t *vec = get_vec (mtx, id);
    assert (count[id] == len(vec));
    free_vec (vec);
  }
}

void transpose_mtx_1 (coll_t *rows, coll_t *cols) {
  uint id = 1, *count = len_cols (rows);
  prealloc_mtx (cols, count, 1);
  fprintf (stderr, "[%.0fs] pre-allocated: %d rows -> %d cols\n",
	   vtime(), nvecs(rows), nvecs(cols)); fflush (stdout);
  while (id <= nvecs(rows)) {
    jix_t *buf = coll_jix (rows, MAP_SIZE/sizeof(jix_t), &id);
    transpose_jix (buf);
    sort_vec (buf, cmp_jix);
    append_jix (cols, buf);
    fprintf (stderr, "[%.0fs] transposed %d rows -> %d cols\n",
	     vtime(), id, nvecs(cols)); fflush (stdout);
    free_vec (buf);
  }
  assert_counts (cols, count);
  free_vec (count);
}

uint next_unmapped_id (coll_t *c, uint start) {
  off_t *offs = c->offs, end = c->vecs->offs + c->vecs->size;
  uint id, n = len(offs);
  for (id = start+1; id < n; ++id) {
    assert (offs[id] >= offs[id-1]);
    if (offs[id] > end) return id-1;
  }
  return n;
}

void transpose_mtx_2 (coll_t *rows, coll_t *cols) {
  uint *df = len_cols (rows), beg, end, cid, rid;
  prealloc_mtx (cols, df, 0);
  uint nr = nvecs(rows), nc = nvecs(cols);
  fprintf (stderr, "[%.0fs] pre-allocated: %d rows -> %d cols\n", vtime(), nr, nc);
  for (end = beg = 1; beg <= nc; beg = end) {
    get_chunk (cols, beg);
    end = next_unmapped_id (cols, beg);
    fprintf (stderr, "[%.0fs] transposing columns %d .. %d\n", vtime(), beg, end);
    for (rid = 1; rid <= nr; ++rid) {
      ix_t *row = mmap_vec (rows, rid, 0, 0), *r, *rEnd = row + len(row);
      for (r = row; r < rEnd; ++r) {
	cid = r->i;
	if (cid < beg || cid >= end) continue; // skip out-of-cache terms
	{
	  uint nid = cols->next[cid];
	  off_t o = cols->offs[cid], n = cols->offs [nid];
	  off_t b = cols->vecs->offs, e = b + cols->vecs->size;
	  assert (o >= b && n <= e);
	}
	ix_t *col = mmap_vec (cols, cid, 0, 0); // already the right size
	ix_t *new = col + len(col) - df[cid]--; //
	new->i = rid;
	new->x = r->x;
      }
    }
  }
  for (cid = 1; cid <= nc; ++cid) assert (df[cid] == 0);
  free_vec (df);
}

*/

// idea: transpose in chunks (like ...mtx_1), merge pairs
void transpose_mtx_merge (coll_t *rows, coll_t *cols) {
  char IN1[9999], IN2[9999], OUT[9999], TMP[9999];
  char *PATH = cols->path ? cols->path : "";
  uint id = 1, nr = num_rows (rows), K = 0;
  ulong pm = physical_memory(), BS = pm/2/sizeof(jix_t);
  while (id <= nr) {
    jix_t *JIX = coll_jix (rows, BS, &id);
    transpose_jix (JIX);
    sort_vec (JIX, cmp_jix);
    coll_t *out = open_coll (fmt(OUT,"%s.%d",PATH,++K), "w+");
    append_jix (out, JIX);
    fprintf (stderr, "[%.0fs] %s: %d rows -> %d cols in %s\n",
	     vtime(), rows->path, id, num_rows(out), OUT);
    free_vec (JIX); free_coll (out);
  }
  while (K > 1) {
    uint k = 0, o = 0, tmp = 0;
    while (k+2 <= K) { // merge pairs: 1+2->1, 3+4->2, ...
      coll_t *in1 = open_coll (fmt(IN1,"%s.%d",PATH,++k), "r+");
      coll_t *in2 = open_coll (fmt(IN2,"%s.%d",PATH,++k), "r+");
      coll_t *out = open_coll (fmt(TMP,"%s.%d",PATH,tmp), "w+");
      uint n1 = num_rows(in1), n2 = num_rows(in2), n = MAX(n1,n2);
      for (id = 1; id <= n; ++id) { // merge: out = in1 + in2
	ix_t *v1 = get_vec (in1, id);
	ix_t *v2 = get_vec (in2, id);
	ix_t *vo = new_vec (0, sizeof(ix_t));
	vo = append_many (vo, v1, len(v1));
	vo = append_many (vo, v2, len(v2));
	if (len(vo)) put_vec (out, id, vo);
	free_vec (v1); free_vec (v2); free_vec (vo);
      }
      free_coll (in1); rm_dir (IN1);
      free_coll (in2); rm_dir (IN2);
      free_coll (out); mv_dir (TMP, fmt(OUT,"%s.%d",PATH,++o));
      fprintf (stderr, "[%.0fs] %s + %s -> %s\n", vtime(), IN1, IN2, OUT);
    }
    if (k < K && o < k) { // odd one at the end
      mv_dir (fmt(IN1,"%s.%d",PATH,++k), fmt(OUT,"%s.%d",PATH,++o));
      fprintf (stderr, "[%.0fs] %s -> %s\n", vtime(), IN1, OUT);
    }
    K = o;
  }
  mv_dir (fmt(OUT,"%s.%d",PATH,K), PATH);
  fprintf (stderr, "[%.0fs] %s -> %s\n", vtime(), OUT, PATH);
}

void *map_vec (coll_t *c, uint id, uint n, uint sz) ;
void transpose_mtx (coll_t *rows, coll_t *cols) {
  cols->rdim = rows->cdim;
  cols->cdim = rows->rdim;
  uint *df = len_cols (rows), nw = len(df), nd = num_rows(rows);
  ulong np = sumi(df), pm = physical_memory(), M = 1<<20;
  pm = MIN(pm,(ulong)MAP_SIZE); // cap [DEBUG]
  ulong BS = MIN(np+1,pm/2/sizeof(ix_t));
  ix_t *buf = calloc (BS, sizeof(ix_t));
  ulong *beg = calloc (nw, sizeof(ulong)), used = 0, done = 0;
  fprintf (stderr, "computed df [%d x %d], will buffer %ldMB",
	   nd, nw-1, BS*sizeof(ix_t)/M);
  uint u=1, v=1, w=1, i=0;
  while (v < nw) {
    for (used = 0; w < nw; ++w) { // [v ... w) fit into buffer
      if (used + df[w] >= BS) break;
      beg[w] = used;
      used += df[w];
    }
    fprintf (stderr, "\nfilling columns %d...%d: %ldM posts\n", v, w-1, used/M);
    for (i = 1; i <= nd; ++i) {
      ix_t *D = get_vec_ro (rows,i), *d;
      for (d = D; d < D+len(D); ++d) {
	if (d->i < v) continue; // skip terms before v
	if (d->i >= w) break; // skip terms after w
	uint b = beg[d->i]++;
	buf[b].i = i;
	buf[b].x = d->x;
	//assert (i && d->x); // DEBUG
	++done;
      }
      show_progress (i, nd, "rows");
    }
    fprintf (stderr, "\nwriting columns %d...%d\n", v, w-1);
    for (u=v; v < w; ++v) {
      //if (df[v] == 0) continue; // added Oct.13 2016 works?
      if (v>u) assert (beg[v] - df[v] == beg[v-1]);
      //ix_t *col = new_vec (df[v], sizeof(ix_t));
      ix_t *col = map_vec (cols, v, df[v], sizeof(ix_t));
      ix_t *src = buf + beg[v] - df[v];
      memcpy (col, src, df[v] * sizeof(ix_t));
      //put_vec (cols, v, col);
      //free_vec (col);
      done -= df[v];
      show_progress (v-u, w-u, "cols");
    }
  }
  fprintf (stderr, " [%.0fs] leftover: %ld of %ld\n", vtime(), (long)done, np);
  //free_vec (buf); free_vec (beg); free_vec (df);
  free (buf); free (beg); free_vec (df);
}

coll_t *transpose (coll_t *M) {
  coll_t *T;
  char *path = M->path ? acat2(M->path,".T") : NULL;
  if (path && coll_exists(path) && coll_modified(path) > coll_modified (M->path))
    T = open_coll (path, "r+");
  else {
    T = open_coll (path, "w+");
    float MB = M->offs[0] / (1<<20), ETA = MB/1400;
    if (path) fprintf (stderr, "transposing %s %.0fMB, ETA %.1f minutes\n", path, MB, ETA);
    transpose_mtx (M,T);
  }
  if (path) free(path);
  return T;
}

float *chk_SCORE (uint N) { // thread-unsafe: static
  static float *SCORE = 0;
  if (!SCORE) SCORE = new_vec (N+1, sizeof(float));
  if (N+1 > len(SCORE)) SCORE = resize_vec (SCORE, N+1);
  return SCORE;
}

ix_t *full2vec (float *full) {
  uint id;
  if (!full) return NULL;
  ix_t *vec = new_vec (0, sizeof(ix_t)), v = {0,0};
  for (id = 1; id < len(full); ++id) {
    if (!full [id]) continue;
    v.i = id;
    v.x = full [id];
    vec = append_vec (vec, &v);
    full [id] = 0;
  }
  return vec;
}

ix_t *double2vec (double *full) {
  uint id;
  if (!full) return NULL;
  ix_t *vec = new_vec (0, sizeof(ix_t)), v = {0,0};
  for (id = 1; id < len(full); ++id) {
    if (!full [id]) continue;
    v.i = id;
    v.x = full [id];
    vec = append_vec (vec, &v);
    full [id] = 0;
  }
  return vec;
}


ix_t *full2vec_keepzero (float *full) {
  uint id;
  if (!full) return NULL;
  ix_t *vec = new_vec (0, sizeof(ix_t)), v = {0,0};
  for (id = 1; id < len(full); ++id) {
    v.i = id;
    v.x = full [id];
    vec = append_vec (vec, &v);
    full [id] = 0;
  }
  return vec;
}

float *vec2full (ix_t *vec, uint N, float def) {
  ix_t *v, *end = vec + len(vec); uint i;
  if (!N && len(vec)) N = (end-1)->i;
  float *full = new_vec (N+1, sizeof(float));
  if (def) for (i=0; i<=N; ++i) full[i] = def;
  for (v = vec; v < end; ++v)
    if (v->i <= N) full [v->i] = v->x;
    else assert (0 && "incorrect dimensions");
  return full;
}

ix_t *ids2vec (uint *U, float x) {
  uint i, n = len(U);
  ix_t *V = const_vec (n, x);
  for (i=0; i<n; ++i) V[i].i = U[i];
  return V;
}

uint *vec2ids (ix_t *V) {
  uint i, n=len(V), *U = new_vec (n, sizeof(uint));
  for (i=0; i<n; ++i) U[i] = V[i].i;
  return U;
}

inline ix_t *vec_find (ix_t *vec, uint i) {
  int lo = -1, hi = len(vec), mid; // both out of range
  while (hi - lo > 1) {
    mid = (lo + hi) / 2;
    if (i > vec[mid].i) lo = mid;
    else                hi = mid;
  }
  if (hi && ((uint)hi == len(vec))) --hi;
  return (i == vec[hi].i) ? (vec + hi) : NULL;
}

inline float vec_get (ix_t *vec, uint id) { // return vec[id]
  ix_t *v = vec_find (vec, id);
  return v ? v->x : 0;
}

inline ix_t *vec_set (ix_t *vec, uint id, float x) { // vec[id] = x
  ix_t *v = vec_find (vec, id), new = {id, x};
  if (v && x) v->x = x; // replace previous value
  else if (v) { v->x = 0; chop_vec (vec); } // remove value
  else if (x) { // insert new value
    vec = append_vec (vec, &new);
    sort_vec (vec, cmp_ix_i);
  }
  return vec;
}

// multiply every row in the matrix by the vector
ix_t *rows_x_vec (coll_t *rows, ix_t *vec) {
  //float *VEC = vec2full (vec, 0);
  ix_t *out = const_vec (nvecs(rows), 0), *o;
  for (o = out; o < out + len(out); ++o) {
    ix_t *row = get_vec (rows, o->i);
    o->x = dot(row,vec);
    //o->x = dot_full(row,VEC);
    free_vec (row);
  }
  //free_vec (VEC);
  return out;
}

// same as cols_x_vec, but for short vecs
ix_t *vec_x_rows (ix_t *vec, coll_t *rows) {
  if (len(vec) == 1 && vec[0].x == 1) return get_vec (rows, vec[0].i);
  ix_t *sum = new_vec (0, sizeof(ix_t)), *v;
  for (v = vec; v < vec + len(vec); ++v) {
    ix_t *row = get_vec (rows, v->i), *tmp = sum;
    sum = vec_add_vec (1, sum, v->x, row);
    free_vec (row); free_vec (tmp);
  }
  return sum;
}

// same, but now we have columns of the matrix
ix_t *cols_x_vec (coll_t *cols, ix_t *vec) {
  float *SCORE = new_vec (cols->cdim + 1, sizeof(float)); // safe
  ix_t *v, *c, *col, *end;
  for (v = vec; v < vec + len(vec); ++v) {
    //#pragma omp critical // no need if cols in memory
    { col = get_vec_mp (cols, v->i); end = col + len(col); }
    //#pragma omp parallel for private(c) // too slow
    for (c = col; c < end; ++c) SCORE [c->i] += v->x * c->x;
    free_vec (col);
  }
  ix_t *result = full2vec (SCORE);
  free_vec (SCORE);
  return result;
}

ix_t *cols_x_vec_overlap (coll_t *cols, ix_t *vec) { // coordination-level match
  ix_t *v; float *SCORE = new_vec (cols->cdim + 1, sizeof(float)); // safe
  for (v = vec; v < vec + len(vec); ++v) {
    if (!v->x) continue;
    ix_t *C = get_vec (cols, v->i), *c = C-1, *end = C+len(C);
    while (++c < end) if (c->x) SCORE [c->i] += 1;
    free_vec (C);
  }
  ix_t *result = full2vec (SCORE);
  free_vec (SCORE);
  return result;
}

void lock (int *x); // in synq.c
void unlock (int *x);

// static SCORE + seen ids
ix_t *cols_x_vec_iseen (coll_t *cols, ix_t *V) {
  static int mutex = 0;
  static float *S = NULL; // persist across calls
  lock (&mutex);
  uint nS = cols->cdim + 1; // largest possible id
  uint nR = nS / 100; // estimate number of results b
  uint *I = new_vec (nR, sizeof(uint)), *i=I, *iEnd = I+len(I);
  S = new_or_resize_vec(S, nS, sizeof(float));
  ix_t *v, *vEnd = V+len(V), *R=NULL;
  for (v = V; v < vEnd; ++v) {
    ix_t *C = get_vec_ro (cols, v->i), *cEnd = C+len(C), *c;
    for (c = C; c < cEnd; ++c) {
      float x = c->x * v->x;
      uint id = c->i;
      if (i < iEnd && !S[id] && x) *i++ = id; // track id
      S[id] += x;
    }
  }
  if (i >= iEnd) R = full2vec(S); // ran out of space in I
  else { // I contains all indices of S with nonzero scores
    len(I) = i-I;
    sort_vec (I, cmp_u);
    R = ids2vec (I,0);
    vec_x_full (R, '=', S);
  }
  free_vec (I);
  unlock (&mutex);
  return R;
}

// static SCORE + skip-lists
ix_t *cols_x_vec_iskip (coll_t *cols, ix_t *V) {
  static int mutex = 0;
  static float *S=NULL; // persist across calls
  static char *A=NULL, *B=NULL, *C=NULL, *D=NULL; // skip-lists
  lock (&mutex);
  const int a=4, b=8, c=12, d=16; //
  uint nS = cols->cdim, i, di = 1;
  S = new_or_resize_vec(S, 1+nS, sizeof(float));
  A = new_or_resize_vec(A, 1+(nS>>a), sizeof(char)); // skip 16 at-a-time
  B = new_or_resize_vec(B, 1+(nS>>b), sizeof(char)); // skip 256
  C = new_or_resize_vec(C, 1+(nS>>c), sizeof(char)); // skip 4096
  D = new_or_resize_vec(D, 1+(nS>>d), sizeof(char)); // skip 65536
  memset (A, 1, len(A));
  memset (B, 1, len(B));
  memset (C, 1, len(C));
  memset (D, 1, len(D));
  ix_t *v, *vEnd = V+len(V);
  for (v = V; v < vEnd; ++v) {
    ix_t *U = get_vec_ro (cols, v->i), *uEnd = U+len(U), *u;
    for (u = U; u < uEnd; ++u) {
      i = u->i;
      S[i] += u->x * v->x; // score
      A[i>>a] = B[i>>b] = C[i>>c] = D[i>>d] = 0; // mark
    }
  }
  ix_t *R = new_vec(0, sizeof(ix_t));
  for (i=1; i<nS; i += di) {
    ix_t new = {i, S[i]};
    if (S[i]) R = append_vec(R, &new);
    di = (D[i>>d] ? (1<<d) :
	  C[i>>c] ? (1<<c) :
	  B[i>>b] ? (1<<b) :
	  A[i>>a] ? (1<<a) : 1);
  }
  unlock (&mutex);
  return R;
}

void rows_x_cols (coll_t *out, coll_t *rows, coll_t *cols) {
  //coll_t *cols = copy_coll (_cols); // in-memory
  uint nr = num_rows (rows), i = 0;
  //#pragma omp parallel for private(i) schedule(dynamic)
  for (i = 1; i <= nr; ++i) {
    ix_t *R=0, *O=0;
    //#pragma omp critical
    { R = get_vec_mp (rows,i); }
    { O = cols_x_vec (cols, R); }
    //#pragma omp critical
    { put_vec (out, i, O); }
    free_vec (R); free_vec (O);
    show_progress (i, nr, "rows");
  }
  //free_coll (cols);
}

void rows_x_rows (coll_t *P, coll_t *A, coll_t *B) {
  uint i = 0;
  while (++i <= num_rows (A)) {
    ix_t *a = get_vec (A, i);
    ix_t *p = rows_x_vec (B, a);
    chop_vec (p);
    put_vec (P, i, p);
    free_vec (a); free_vec (p);
  }
}

void rows_o_vec (coll_t *out, coll_t *rows, char op, ix_t *vec) {
  uint n = num_rows (rows), id;
  for (id = 1; id <= n; ++id) {
    ix_t *a = get_vec (rows, id);
    ix_t *b = vec_x_vec (a, op, vec);
    put_vec (out, id, b);
    free_vec (a); free_vec (b);
  }
}

void rows_o_rows (coll_t *out, coll_t *A, char op, coll_t *B) {
  uint nA = num_rows (A), nB = num_rows (B), n = MAX(nA,nB), id;
  for (id = 1; id <= n; ++id) {
    ix_t *a = get_vec_ro (A, id);
    ix_t *b = get_vec_ro (B, id);
    ix_t *p = vec_x_vec (a, op, b);
    put_vec (out, id, p);
    free_vec (p);
  }
}

void rows_a_rows (coll_t *out, float wa, coll_t *A, float wb, coll_t *B) {
  uint nA = num_rows (A), nB = num_rows (B), n = MAX(nA,nB), id;
  for (id = 1; id <= n; ++id) {
    ix_t *a = get_vec_ro (A, id);
    ix_t *b = get_vec_ro (B, id);
    ix_t *p = vec_add_vec (wa, a, wb, b);
    put_vec (out, id, p);
    free_vec (p);
  }
}

void filter_rows (coll_t *rows, char op, ix_t *mask) {
  uint n = num_rows (rows), id;
  for (id = 1; id <= n; ++id) {
    ix_t *R = get_vec_ro (rows, id);
    if (op == '&') filter_and (R, mask);
    else           filter_not (R, mask);
  }
}

void rows_x_num (coll_t *rows, char op, double num) {
  uint n = num_rows (rows), id;
  for (id = 1; id <= n; ++id) {
    ix_t *R = get_vec_ro (rows, id); // in-place
    vec_x_num (R, op, num); // in-place
  }
}

// Lp-norm, chi-squared (X2), Bhattacharyya Coefficient (BC)
/*
ix_t *cols_x_vec_prm (coll_t *cols, ix_t *vec, float *C,
		      float p, char *LP, char *X2, char *BC) {
  float *S = chk_SCORE (0), V = sump (p,vec);
  ix_t *v, *c;
  for (v = vec; v < vec+len(vec); ++v) {
    ix_t *col = get_vec (cols, v->i), *e = col+len(col), *c=col-1;
    S = chk_SCORE (last_id(col));
    if (LP) while (++c < e) S[c->i] += norm(p,v->x - c->x) - norm(p,v->x) - norm(p,c->x);
    if (X2) while (++c < e) S[c->i] += 1 / (V / v->x + C[c->i] / c->x);
    if (BC) while (++c < e) S[c->i] += sqrt ((v->x / V) * (c->x / C[c->i]));
    free_vec (col);
  }
  ix_t *out = full2vec(S), *end = out+len(out), *o = out-1;
  if (bc || x2) return out;
  if (nr) while (++o < end) o->x =            o->x + Vp + Cp[o->i];
  else    while (++o < end) o->x = norm (1/p, o->x + Vp + Cp[o->i]);
  return out;
}
*/

/*
void mtx_x_mtx (coll_t *P, coll_t *A, coll_t *B, float(*F)(ix_t*,ix_t*)) {
  uint i, nA = nvecs(A), nB = nvecs(B);
  for (i = 1; i <= nA; ++i) {
    ix_t *a = get_vec (A,i);
    ix_t *p = rows_x_vec (B,a);
    put_vec (P, i, p);
    free_vec (a);
    free_vec (p);
  }
}

//              N x K      N x D      D x K
void A_x_T (coll_t *P, coll_t *A, coll_t *T) {
  uint i, nA = nvecs(A);
  for (i = 1; i <= nA; ++i) {
    ix_t *a = get_vec (A, i), *v;
    put_vec (P, i, p);
    free_vec (p);
    free_vec (a);
  }
}
*/

/*
//  data[NxD] = input
// means[KxD] = random
//  sims[NxK] = data x means.T
//  prob[NxK] = posterior sims
// means[KxD] = prob.T x data
//               N x D      K x D      K x N
void kmeans (coll_t *X, coll_t *M, coll_t *P, uint iter) {
  while (--iter > 0) {
    A_x_B (P, M, X, cosine);
    A_x_T (N, P, X);
  }
}
*/

double dot_full (ix_t *vec, float *full) {
  double s = 0; uint n = len(full);
  ix_t *v = vec-1, *end = vec+len(vec);
  while (++v < end && v->i < n) s += v->x * full[v->i];
  return s;
}

double dot_dense (ix_t *vec, ix_t *dense) {
  double s = 0; uint n = len(dense);
  ix_t *v = vec-1, *end = vec+len(vec);
  while (++v < end && v->i <= n) s += v->x * dense[v->i-1].x;
  return s;
}

double dot (ix_t *A, ix_t *B) {
  if (!A || !B) return 0;
  else if (is_dense(A)) return dot_dense (B,A);
  else if (is_dense(B)) return dot_dense (A,B);
  double s = 0;
  ix_t *a = A, *b = B, *aEnd = A + len(A), *bEnd = B + len(B);
  for (; a < aEnd; ++a) { // for each a.i
    while ((b < bEnd) && (b->i < a->i)) ++b; // skip b.i < a.i
    if (b >= bEnd) break;
    if (a->i == b->i) s += a->x * b->x;
  }
  return s;
}

double kdot (uint k, ix_t *A, ix_t *B) {
  ix_t *C = vec_x_vec (A, '.', B);  trim_vec (C, k);
  double result = sum(C);           free_vec (C);
  return result;
}

double pdot (float p, ix_t *A, ix_t *B) {
  double norm = 0, dx = 0;
  ixy_t *C = join (A, B, 0), *end = C + len(C), *c = C-1;
  if      (p==2)  while (++c < end) { dx = c->x * c->y; norm += dx * dx;   }
  else if (p==1)  while (++c < end) { dx = c->x * c->y; norm += dx;        }
  else if (p==.5) while (++c < end) { dx = c->x * c->y; norm += sqrt(dx);  }
  else if (p==0)  while (++c < end) { dx = c->x * c->y; norm += (dx != 0); }
  else            while (++c < end) { dx = c->x * c->y; norm += powf(dx,p);}
  free_vec (C);
  return (p==2) ? sqrt(norm) : (p==1 || p==0) ? norm : powf(norm,1/p);
}

double pnorm (float p, ix_t *A, ix_t *B) {
  double norm = 0, dx = 0;
  ixy_t *C = join (A, B, 0), *end = C + len(C), *c = C-1;
  if      (p==2)  while (++c < end) { dx = ABS(c->x - c->y); norm += dx * dx;       }
  else if (p==1)  while (++c < end) { dx = ABS(c->x - c->y); norm += ABS(dx);       }
  else if (p==0)  while (++c < end) { dx = ABS(c->x - c->y); norm += (dx!=0);       }
  else if (p==.5) while (++c < end) { dx = ABS(c->x - c->y); norm += sqrt(ABS(dx)); }
  else            while (++c < end) { dx = ABS(c->x - c->y); norm += powa(dx,p);    }
  free_vec (C);
  return (p==2) ? sqrt(norm) : (p==1 || p==0) ? norm : powa(norm,1/p);
}

double pnorm_full (float p, ix_t *A, ix_t *B) { // faster version for full vectors
  double norm = 0, dx = 0;
  uint i = 0, n = len(A), m = len(B);
  assert ((n==m) && "full vector dimensions must agree");
  if      (p==2)  for (; i<n; ++i) { dx = ABS(A[i].x - B[i].x); norm += dx * dx;   }
  else if (p==1)  for (; i<n; ++i) { dx = ABS(A[i].x - B[i].x); norm += ABS(dx);   }
  else if (p==0)  for (; i<n; ++i) { dx = ABS(A[i].x - B[i].x); norm += (dx!=0);   }
  else if (p==.5) for (; i<n; ++i) { dx = ABS(A[i].x - B[i].x); norm += sqrt(ABS(dx)); }
  else            for (; i<n; ++i) { dx = ABS(A[i].x - B[i].x); norm += powa(dx,p); }
  return (p==2) ? sqrt(norm) : (p==1 || p==0) ? norm : powa(norm,1/p);
}

float pnorm_semi (float p, float *A, ix_t *B, float normA) {
  assert (last_id(B) < len(A));
  ix_t *b = B-1, *end = B+len(B);
  while (++b<end) normA += powa((A[b->i]-b->x),p) - powa(A[b->i],p);
  return (p==1 || p==0) ? normA : powa(normA,1/p);
}

double wpnorm (float p, ix_t *A, ix_t *B, ix_t *W) {
  ix_t *D = vec_x_vec (A,'-',B), *d;
  for (d = D; d < D+len(D); ++d) d->x = powa(d->x,p);
  double norm = dot (W,D);
  free_vec (D);
  return norm;
  //return (p==2) ? sqrt(norm) : (p==1 || p==0) ? norm : powa(norm,1/p);
}

ix_t *max (ix_t *V) {
  ix_t *end = V + len(V), *v = V, *m = V;
  for (; v < end; ++v) if (m->x < v->x) m = v;
  return m; }

ix_t *min (ix_t *V) {
  ix_t *end = V + len(V), *v = V, *m = V;
  for (; v < end; ++v) if (m->x > v->x) m = v;
  return m; }

double sum (ix_t *V) {
  ix_t *end = V + len(V), *v = V;
  double s = 0;
  for (; v < end; ++v) s += v->x;
  return s; }

double sum2 (ix_t *V) {
  ix_t *end = V + len(V), *v = V;
  double s = 0;
  for (; v < end; ++v) s += v->x * v->x;
  return s; }

double sump (float p, ix_t *V) {
  ix_t *end = V + len(V), *v = V-1;
  double s = 0;
  if      (p == 1) while (++v < end) s += v->x;
  else if (p == 2) while (++v < end) s += v->x * v->x;
  else if (p == 0) while (++v < end) s += (v->x != 0);
  else if (p ==.5) while (++v < end) s += sqrt (v->x);
  else             while (++v < end) s += powf (v->x,p);
  return s;
}

double sum_slice (ix_t *V, uint n) {
  ix_t *end = V + n, *v = V-1;
  double s = 0;
  while (++v < end) s += v->x;
  return s;
}

double sum2_slice (ix_t *V, uint n) {
  ix_t *end = V + n, *v = V-1;
  double s = 0;
  while (++v < end) s += v->x * v->x;
  return s;
}

double norm (float p, float root, ix_t *V) {
  ix_t *end = V + len(V), *v = V-1;
  double s = 0;
  if      (p == 1) while (++v < end) s += ABS(v->x);
  else if (p == 2) while (++v < end) s += v->x * v->x;
  else if (p == 0) while (++v < end) s += (v->x != 0);
  else if (p ==.5) while (++v < end) s += sqrt (ABS(v->x));
  else             while (++v < end) s += powa (v->x,p);
  return root ? powa(s,root) : s;
}

// assumes X[0:n) is sorted
double median (ix_t *X, uint n) {
  uint m = n>>1; // midpoint
  if (n & 1) return X[m].x;
  else return (X[m].x + X[m+1].x) / 2;
}

double mean(ix_t *X) { return sum(X) / len(X); }

double stdev(ix_t *X) { return sqrt(variance(X, 0)); }

double variance (ix_t *X, uint n) {
  if (!n) n = len(X);
  if (n < 2) return 0;
  double EX = sum(X) / n;
  double EX2 = sum2(X) / n;
  return (EX2 - EX * EX) * n / (n-1); // Bessel's correction
}

// returns mean and standard deviation of values in X
xy_t mean_and_stdev_of_vec(ix_t *X) {
  uint n = len(X);
  if (n < 2) return (xy_t) {(n ? X->x : 0), 0};
  double EX = sum(X) / n;
  double EX2 = sum2(X) / n;
  double VX = (EX2 - EX * EX) * n / (n-1); // Bessel
  return (xy_t) {EX, sqrt(VX)};
}

// mean and st.dev of p-confidence interval of X
xy_t mean_and_stdev(ix_t *_X, float p) {
  ix_t *X = copy_vec(_X);
  xy_t CI = median_interval(X, p);
  vec_x_range(X, '.', CI); // drop outliers
  xy_t result = mean_and_stdev_of_vec(X);
  free_vec(X);
  return result;
}

ulong sumi (uint *V) {
  uint *end = V+len(V), *v = V-1;
  ulong s = 0;
  while (++v < end) s += *v;
  return s; }

uint *maxi (uint *V) {
  uint *end = V + len(V), *v = V-1, *m = V;
  while (++v < end) if (*v > *m) m = v;
  return m; }

double sumf (float *V) {
  float *end = V + len(V), *v = V-1;
  double s = 0;
  while (++v < end) s += *v;
  return s; }

float *maxf (float *V) {
  float *end = V + len(V), *v = V-1, *m = V;
  while (++v < end) if (*v > *m) m = v;
  return m; }

float *minf (float *V) {
  float *end = V + len(V), *v = V-1, *m = V;
  while (++v < end) if (*v < *m) m = v;
  return m; }

uint count (ix_t *V, char op, float x) {
  uint n = 0;
  ix_t *v = V-1, *end = V+len(V);
  if      (op == '=') while (++v < end) n += (v->x == x);
  else if (op == '>') while (++v < end) n += (v->x  > x);
  else if (op == '<') while (++v < end) n += (v->x  < x);
  else if (op == '!') while (++v < end) n += (v->x != x);
  return n;
}

// sort X high-to-low, return deltas between adjacent values
ix_t *value_deltas(ix_t *X) {
  if (!X || len(X) < 1) return NULL;
  ix_t *D = copy_vec(X);
  sort_vec(D, cmp_ix_X);
  uint i, n = len(D)-1;
  for (i=0; i<n; ++i) D[i].x = D[i].x - D[i+1].x;
  len(D) = n;
  return D;
}

// sort V, collapse duplicates, replace id by count of eps-dups
ix_t *distinct_values (ix_t *V, float eps) {
  ix_t *a, *b, *C = copy_vec(V), *end = C+len(C);
  sort_vec (C, cmp_ix_x);
  for (a = b = C; b < end; ++b) {
    b->i = 1;
    if (a == b) continue;
    float diff = b->x ? (a->x / b->x - 1) : 1;
    float same = ABS(diff) <= eps;
    if (same) { a->i += b->i; a->x = b->x; }
    else if (++a < b) *a = *b;
  }
  return resize_vec (C, a - C + 1);
}

double value_entropy (ix_t *V) {
  ix_t *C = distinct_values (V,.01), *c;
  double N = len(V), H = 0;
  for (c = C; c < C+len(C); ++c) {
    double p = c->i / N;
    H -= p * log2(p);
  }
  free_vec (C);
  return H;
}

double lnP_X_BG (ix_t *X, float *CF, ulong CL) {
  double lnP = 0;
  ix_t *w, *end = X + len(X);
  for (w = X; w < end; ++w) {
    double Pw = CF[w->i] / (double)CL;
    lnP += w->x * log (Pw);
  }
  return lnP;
}

double lnP_X_Y (ix_t *X, ix_t *Y, double mu, float *CF, ulong CL) {
  double SY = sum(Y), lnP = 0;
  ixy_t *W = join (X, Y, 0), *w, *end = W + len(W);
  for (w = W; w < end; ++w) {
    double Pw = CF[w->i] / (double)CL;
    double PwY = (w->y + mu * Pw) / (SY + mu);
    lnP += w->x * log (PwY);
  }
  free_vec (W);
  return lnP;
}

double lnP (ix_t *X, ix_t *Y, double mu, float *CF, ulong CL) {
  return (Y && len(Y)) ? lnP_X_Y (X,Y,mu,CF,CL) : lnP_X_BG (X, CF, CL);
}

double entropy (ix_t *X) {
  double SX = sum(X), entropy = 0;
  ix_t *w, *end = X + len(X);
  for (w = X; w < end; ++w) {
    double p = w->x / (SX ? SX : 1);
    entropy -= p * log(p);
  }
  return entropy;
}

double cross_entropy (ix_t *X, ix_t *Y, double mu, float *CF, ulong CL) {
  return -lnP (X,Y,mu,CF,CL) / sum(X);
}

double KL (ix_t *X, ix_t *Y, double mu, float *CF, ulong CL) {
  return cross_entropy (X,Y,mu,CF,CL) - entropy (X);
}

double LR (ix_t *X, ix_t *Y, double mu, float *CF, ulong CL) {
  return (lnP (X,Y,mu,CF,CL) - lnP (X,0,mu,CF,CL)) / sum(X);
}

double cosine (ix_t *X, ix_t *Y) {
  double norm = sqrt (sum2(X) * sum2(Y));
  return dot(X,Y) / (norm ? norm : 1);
}

double dice (ix_t *X, ix_t *Y) {
  double norm = sum2(X) + sum2(Y);
  return 2 * dot(X,Y) / (norm ? norm : 1);
}

double jaccard (ix_t *X, ix_t *Y) {
  double D = dot(X,Y), norm = sum2(X) + sum2(Y) - D;
  return D / (norm>0 ? norm : 1);
}

// sum_i sqrt (Xi/|X| * Yi/|Y|) = sum_i sqrt (Xi*Yi) / sqrt (|X|*|Y|)
double hellinger (ix_t *X, ix_t *Y) {
  double norm = sqrt (sum(X) * sum(Y));
  return pdot(.5,X,Y) / (norm>0 ? norm : 1);
}

double chi2 (ix_t *X, ix_t *Y) {
  double chi2 = 0, sX = sum(X), sY = sum(Y), sumX = sX?sX:1, sumY = sY?sY:1;
  ixy_t *C = join (X, Y, 0), *end = C + len(C), *c = C-1;
  while (++c < end) {
    double x = c->x / sumX, y = c->y / sumY;
    if (x+y) chi2 += (x-y) * (x-y) / (x+y);
  }
  free_vec (C);
  return chi2 / 2;
}

double chi2_full (ix_t *A, ix_t *B) {
  double chi2 = 0, sumA = sum(A), sumB = sum(B);
  uint i, n = len(A), m = len(B);
  assert ((n==m) && "full vector dimensions must agree");
  for (i=0; i<n; ++i) {
    double a = A[i].x / sumA, b = B[i].x / sumB;
    if (a+b) chi2 += (a-b) * (a-b) / (a+b);
  }
  return chi2/2;
}

double covariance (ix_t *X, ix_t *Y, uint N) {
  if (!N) N = MAX(len(X),len(Y));
  double EX = sum(X)/N, EY = sum(Y)/N, EXY = dot(X,Y)/N;
  return EXY - EX*EY;
}

double MSE (ix_t *X, ix_t *Y, uint N) {
  return (sum2(X) + sum2(Y) - 2*dot(X,Y)) / N;
}

double correlation (ix_t *X, ix_t *Y, double N) {
  double VX = variance(X,N), VY = variance(Y,N);
  double VXY = covariance(X,Y,N), norm = sqrt (VX*VY);
  return VXY / (norm ? norm : 1);
}

double rsquared (ix_t *Y, ix_t *F, double N) {
  //double SStot = variance (Y,N);
  //double SSres = MSE (Y,F,N);
  double SY2 = sum2(Y), SY = sum(Y);
  double SF2 = sum2(F), SFY = dot(F,Y);
  double SStot = SY2 - SY * SY / N;
  double SSres = SY2 + SF2 - 2*SFY;
  double R2 = 1 - SSres / SStot;
  //fprintf (stderr, "SStot: %f SSres: %f R2: %f\n", SStot, SSres, 1 - SSres / SStot);
  return R2;
}

double igain (ix_t *score, ix_t *truth, double N) {
  ixy_t *words = join (score, truth, 0), *w, *wEnd = words+len(words);
  double n01 = 1, n10 = 1, n11 = 1, n00 = 1+MAX(0,N-len(words));
  for (w = words; w < wEnd; ++w)
    if (w->x && w->y) ++n11;
    else if    (w->x) ++n10;
    else if    (w->y) ++n01;
    else              ++n00;
  free_vec (words);
  double n1 = n11+n10, n0 = n01+n00, n_1 = n01+n11, n_0 = n00+n10, n = n1+n0;
  double H1 = n1 * log(n1) - n11 * log(n11) - n10 * log(n10);
  double H0 = n0 * log(n0) - n01 * log(n01) - n00 * log(n00);
  double H  =  n * log (n) - n_1 * log(n_1) - n_0 * log(n_0);
  return (H - H0 - H1) / n;
}

uint num_rel (ixy_t *evl) {
  uint n = 0; ixy_t *e = evl-1, *end = evl+len(evl);
  while (++e < end) if (e->y > 0) ++n;
  return n; }

uint num_ret (ixy_t *evl) {
  uint n = 0; ixy_t *e = evl-1, *end = evl+len(evl);
  while (++e < end) if (e->x > -Infinity) ++n;
  return n; }

uint rel_ret (ixy_t *evl) {
  uint n = 0; ixy_t *e = evl-1, *end = evl+len(evl);
  while (++e < end) if (e->y > 0 && e->x > -Infinity) ++n;
  return n; }

double evl_AP (ixy_t *evl) {
  double AP = 0, RR = 0, rel = num_rel (evl);
  ixy_t *e = evl-1, *end = evl+len(evl);
  while (++e < end) if (e->y > 0 && e->x > -Infinity) AP += (++RR / (e-evl+1));
  return AP / rel;
}

double evl_AUC (ixy_t *evl, double neg) {
  double AUC = 0, pos = num_rel(evl), TN=neg;
  assert (pos+neg >= len(evl));
  ixy_t *e = evl-1, *end = evl+len(evl);
  while (++e < end)
    if (e->y <= 0) --TN; // false positive
    else if (e->x > -Infinity) AUC += TN; // true positive
  return AUC / (pos * neg);
}

xy_t maxF1 (ix_t *system, ix_t *truth, double b) {
  xy_t best = {0,0}; // b=99 ignores Precision, b=0.01 ignores Recall
  ixy_t *evl = join (system, truth, -Infinity);
  sort_vec (evl, cmp_ixy_X); // sort: descending system score
  double RR = 0, rel = num_rel (evl);
  ixy_t *e = evl-1, *end = evl+len(evl);
  while (++e < end) if (e->y > 0 && e->x > -Infinity) {
      double R = ++RR / rel;
      double P = RR / (e-evl+1);
      double F = (b*b+1)*P*R / (b*b*P+R);
      if (F > best.y) best = (xy_t) {e->x, F};
    }
  free_vec (evl);
  return best;
}

/*
// return {i,x,y} = best {feature, threshold, F1}
ixy_t rules_cut (coll_t *XX, ix_t *Y, ix_t *mask, float b, char low) {
  if (mask) filter_and (Y=copy_vec(Y), mask);
  ixy_t best = {0,0,0};
  uint i, n = num_rows(XX);
  for (i = 1; i<= n; ++i) {
    ix_t *X = get_vec(XX,i);
    if (mask) filter_and(X,mask);
    if (low) num_x_vec(0,'-',X);      // negate if ascending
    xy_t this = maxF1(X,Y,b);         // X  thresh  F-value
    if (this.y > best.y) best = (ixy_t) {i, this.x, this.y};
    free_vec(X);
  }
  if (mask) free_vec(Y);
  return best;
}

ixy_t *rules_chain (coll_t *XX, ix_t *Y, char *prm) {
  ixy_t RA = rule_cut(XX,Y,100,0); // -----|-+-+-+- +++++ recall above thr
  ixy_t PA = rule_cut(XX,Y,.01,0); // ----- -+-+-+-|+++++ precision above
  ixy_t RZ = rule_cut(XX,Y,100,1); // +++++ -+-+-+-|----- recall below thr
  ixy_t PZ = rule_cut(XX,Y,.01,1); // +++++|-+-+-+- ----- precision below
  ixy_t best = RA;
  if (PA.y > best.y) best = PA;
  if (RZ.y > best.y) best = RZ;
  if (PZ.y > best.y) best = PZ;
}
*/

double F1 (ix_t *system, ix_t *truth) {
  ixy_t *evl = join (system, truth, -Infinity);
  double rel = num_rel(evl), ret = num_ret(evl), relret = rel_ret(evl);
  double P = relret / ret, R = relret / rel, b = 1;
  free_vec (evl);
  return (b+1) * P * R / (b * P + R); // F_beta
}

double AveP (ix_t *system, ix_t *truth) {
  ixy_t *evl = join (system, truth, -Infinity);
  sort_vec (evl, cmp_ixy_X); // sort by decreasing system scores
  double AP = evl_AP (evl);
  free_vec (evl);
  return AP;
}

double weighted_ap (ix_t *system, ix_t *truth) {
  ixy_t *evl = join (system, truth, -Infinity), *e;
  sort_vec (evl, cmp_ixy_X); // sort by decreasing system scores
  double sumPrec = 0, relret = 0, rel = sum (truth);
  for (e = evl; e < evl + len(evl); ++e)
    if (e->y > 0 && e->x > -Infinity) // rel docs in the ranking
      sumPrec += (relret += e->y) / (e-evl+1);
  free_vec (evl);
  return sumPrec / rel;
}

void eval_dump_evl (FILE *out, uint id, ixy_t *evl) {
  uint numrel = num_rel(evl); ixy_t *e;
  fprintf (out, "%d %d", id, numrel);
  for (e = evl; e < evl+len(evl); ++e) {
    uint rank = (e->x > -Infinity) ? (e - evl + 1) : (e - evl + 1E4);
    if (e->y > 0) fprintf (out, " %d", rank);
  }
  fprintf (out, " /\n");
}

void eval_dump_roc (FILE *out, uint id, ixy_t *evl, float b) {
  double FP = 0, TP = 0, nP = num_rel(evl), nN = len(evl) - nP; ixy_t *e;
  fprintf (out, "#%5s %6s %6s   F%3.1f %3s %10s %1s query:%d %.0f+ %.0f-\n",
	   "TPR", "FPR", "Prec", b, "rnk", "score", "R", id, nP, nN);
  for (e = evl; e < evl+len(evl); ++e) {
    if (e->y > 0) ++TP; else ++FP;
    double rnk = TP+FP, TPR = TP/nP, FPR = FP/nN, Prec = TP/rnk;
    double F1 = (b*b+1)*TPR*Prec / (b*b*Prec + TPR);
    fprintf (out, "%6.4f %6.4f %6.4f %6.4f %3.0f %10.4f %1s\n",
	     TPR, FPR, Prec, F1, rnk, e->x, ((e->y>0)?"1":"0"));
  }
  //free_vec (evl);
}

void eval_dump_map (FILE *out, uint id, ixy_t *evl, char *prm) { // thread-unsafe: static
  static double mP = 0, mR = 0, mF1 = 0, mAP = 0, mRe = 0, mTP = 0, n = 0;
  if (evl) {
    double rel = num_rel(evl), ret = num_ret(evl), TP = rel_ret(evl);
    double P = TP / ret, R = TP / rel, b = getprm(prm,"b=",1);
    double F1 = (b+1) * P * R / (b * P + R);
    double AP = evl_AP (evl);
    n += 1; mRe += rel; mTP += TP; mP += P; mR += R; mF1 += F1; mAP += AP;
    fprintf (out, "%4d %4.0f %4.0f %.4f %.4f %.4f %.4f\n", id, rel, TP, R, P, F1, AP);
  } else if (n) {
    fprintf (out, "%4s %4.0f %4.0f %.4f %.4f %.4f %.4f\n", "avg", mRe, mTP, mR, mP, mF1, mAP);
    mP = mR = mF1 = mAP = mRe = mTP = n = 0;
  } else
    fprintf (out, "%4s %4s %4s %6s %6s %-6s %-6s\n", "Qry", "Rel", "Rret", "Recall", "Precis", "F1", "AveP");
}

float *mtx_full_row (char *_M, uint row) {
  coll_t *M = open_coll (_M, "r+");
  ix_t *_row = get_vec_ro (M,row);
  float *F = vec2full (_row, num_cols(M), 0);
  free_coll(M);
  return F;
}

ixy_t *join (ix_t *X, ix_t *Y, float def) {
  ix_t *x = X, *y = Y, *endX = X + len(X), *endY = Y + len(Y);
  ixy_t *Z = new_vec (len(X)+len(Y), sizeof(ixy_t)), *z = Z;
  while (x < endX && y < endY) {
    if      (y->i < x->i) { z->i = y->i; z->x =  def; z->y = y->x; ++z; ++y; }
    else if (x->i < y->i) { z->i = x->i; z->x = x->x; z->y =  def; ++z; ++x; }
    else                  { z->i = x->i; z->x = x->x; z->y = y->x; ++z; ++x; ++y; }
  }
  while (x < endX) { z->i = x->i; z->x = x->x; z->y =  def; ++z; ++x; }
  while (y < endY) { z->i = y->i; z->x =  def; z->y = y->x; ++z; ++y; }
  return resize_vec (Z, z-Z);
}

void disjoin (ix_t *X, ix_t *Y) {
  ixy_t *Z = join (X, Y, -Infinity), *zEnd = Z + len(Z), *z;
  ix_t *x = X, *y = Y;
  for (z = Z; z < zEnd; ++z)
    if (z->x > z->y) { x->i = z->i; x->x = z->x; ++x; }
    else             { y->i = z->i; y->x = z->y; ++y; }
  assert (x < X+len(X) && y < Y+len(Y));
  free_vec (Z);
}

char *vec2set (ix_t *vec, uint N) {
  ix_t *v, *end = vec + len(vec);
  if (!N && len(vec)) N = (end-1)->i;
  else assert (N >= (end-1)->i);
  char *set = new_vec (N+1, sizeof(char));
  for (v = vec; v < end; ++v) if (v->x) set [v->i] = 1;
  return set;
}

void vec_x_set (ix_t *vec, char op, char *set) {
  assert (vec && set && (op == '.' || op == '*' || op == '-'));
  uint N = len(set);
  ix_t *u = vec, *v = vec-1, *end = vec+len(vec);
  if (end > vec) assert (end[-1].i < N);
  if (op == '*') while (++v < end) if ( set[v->i]) *u++ = *v;
  if (op == '.') while (++v < end) if ( set[v->i]) *u++ = *v;
  if (op == '-') while (++v < end) if (!set[v->i]) *u++ = *v;
  len(vec) = u - vec;
}

void filter_and (ix_t *V, ix_t *F) {
  if (!V || !F) return;
  ix_t *v = V, *f = F, *endV = V + len(V), *endF = F + len(F);
  while (v < endV && f < endF) {
    if      (v->i >  f->i) { ++f; } // f but not v => ignore
    else if (v->i <  f->i) { v->i = 0;  ++v; } // v not in f => skip
    else if (v->i == f->i) { ++v; ++f; } // v in f => keep it
  }
  len(V) = v-V; // drop remaining elements in V (if any)
  chop_vec (V);
}

void filter_not (ix_t *V, ix_t *F) {
  if (!V || !F) return;
  ix_t *v = V, *f = F, *endV = V + len(V), *endF = F + len(F);
  //fprintf (stderr, "# V[%d] - F[%d]", len(V), len(F));
  //uint hit = 0;
  while (v < endV && f < endF) {
    if      (v->i >  f->i) { ++f; } // f but not v => ignore
    else if (v->i <  f->i) { ++v; } // v not in f => keep
    else if (v->i == f->i) { v->i = 0; ++v; ++f; } // v in f => skip
  } // keep remaining elements in V (they're not in F)
  //fprintf (stderr, " -> V[%d]", len(V));
  //uint leftV = endV - v, leftF = endF - f;
  chop_vec (V);
  //fprintf (stderr, " -> V[%d] hit: %d leftV: %d leftF: %d\n", len(V), hit, leftV, leftF);
}

void filter_set (ix_t *V, ix_t *F, float def) {
  if (!V || !F) return;
  ix_t *v = V, *f = F, *endV = V + len(V), *endF = F + len(F);
  while (v < endV && f < endF) {
    if      (v->i >  f->i) { ++f; } // f but not v => ignore
    else if (v->i <  f->i) { v->x = def;  ++v; } // v not in f => default
    else if (v->i == f->i) { v->x = f->x; ++v; ++f; } // v in f =>
  }
  while (v < endV) { v->x = def; ++v; } // set remains of V to default
  chop_vec (V);
}

void filter_and_sum (ix_t *V, ix_t *F) { // Boolean AND + SUM the scores
  if (!V || !F) return;
  ix_t *v = V, *f = F, *endV = V + len(V), *endF = F + len(F);
  while (v < endV && f < endF) {
    if      (v->i >  f->i) { ++f; } // f but not v => ignore
    else if (v->i <  f->i) { v->i = 0;  ++v; } // v not in f => skip
    else if (v->i == f->i) { v->x += f->x; ++v; ++f; } // v in f => SUM
  }
  len(V) = v-V;
  chop_vec (V);
}

void filter_sum (ix_t **V, ix_t *F) { // OR + sum scores + free old vec
  ix_t *new = vec_x_vec (*V, '+', F);
  free_vec (*V); *V = new;
}

void vec_mul_vec (ix_t *V, ix_t *F) {
  if (!V || !F) return;
  ix_t *v = V, *f = F, *endV = V + len(V), *endF = F + len(F);
  while (v < endV && f < endF) {
    if      (f->i < v->i) { ++f; }           // y but not x
    else if (v->i < f->i) { v->i = 0; ++v; } // x but not y
    else                  { v->x *= f->x; ++v; ++f; }
  }
  len(V) = v-V;
  chop_vec (V);
}

ix_t *jix_to_ix_and_free (jix_t *buf, uint *id) {
  jix_t *b, *end = buf + len(buf);
  assert (id && (end > buf));
  *id = buf->j;
  ix_t *vec = new_vec((end-buf), sizeof(ix_t)), *v;
  for (b = buf, v = vec; b < end; ++b,++v) {
    //if (b->j != *id) {
    //  fprintf (stderr, "jix: %d != %d %d %f\n", *id, b->j, b->i, b->x);
    //  continue;
    //}
    assert ((b->j == *id) && "buf should contain only one vector");
    v->i = b->i;
    v->x = b->x;
  }
  free_vec (buf);
  resize_vec (vec, v-vec);
  sort_uniq_vec (vec);
  chop_vec (vec);
  return vec;
}

ixy_t *ix2ixy (ix_t *ix, float y) {
  ix_t *v; ixy_t *ixy = new_vec (0, sizeof(ixy_t));
  for (v = ix; v < ix+len(ix); ++v) {
    ixy_t new = {v->i, v->x, y};
    ixy = append_vec (ixy, &new);
  }
  return ixy;
}

ix_t *ixy_to_ix_and_free (ixy_t *Z) {
  ixy_t *z, *end = Z + len(Z);
  ix_t *C = new_vec (len(Z), sizeof(ix_t)), *c = C;
  for (z = Z; z < end; ++z) { c->i = z->i; c->x = z->x; ++c; }
  //      if (z->i && z->x) { c->i = z->i; c->x = z->x; ++c; }
  free_vec (Z);
  //return resize_vec (C, c-C);
  return C;
}

ix_t *ixy_to_ix (ixy_t *Z) {
  ixy_t *z, *end = Z + len(Z);
  ix_t *C = new_vec (len(Z), sizeof(ix_t)), *c = C;
  for (z = Z; z < end; ++z, ++c) { c->i = z->i; c->x = z->x; }
  return C;
}

ix_t *vec_add_vec (float x, ix_t *X, float y, ix_t *Y) {
  ixy_t *Z = join (X, Y, 0), *end = Z + len(Z), *z = Z;
  for (; z < end; ++z) z->x = x * z->x + y * z->y;
  return ixy_to_ix_and_free (Z);
}

void vec_x_full (ix_t *V, char op, float *F) {
  ix_t *v = V-1, *last = V+len(V);
  while (--last >= V && last->i >= len(F)) last->x = 0;
  switch (op) {
  case '+': while (++v <= last) v->x += F [v->i];                          break;
  case '-': while (++v <= last) v->x -= F [v->i];                          break;
  case '*': while (++v <= last) v->x *= F [v->i];                          break;
  case '.': while (++v <= last) v->x *= F [v->i];                          break;
  case '/': while (++v <= last) v->x /= F [v->i];                          break;
  case '^': while (++v <= last) v->x = powf (v->x, F [v->i]);              break;
  case '<': while (++v <= last) v->x = (v->x < F [v->i]) ? 1 : 0;          break;
  case '>': while (++v <= last) v->x = (v->x > F [v->i]) ? 1 : 0;          break;
  case 'm': while (++v <= last) v->x = (v->x < F [v->i]) ? v->x : F[v->i]; break;
  case 'M': while (++v <= last) v->x = (v->x > F [v->i]) ? v->x : F[v->i]; break;
  case 't': while (++v <= last) v->x = (v->x <=F [v->i]) ? v->x : 0;       break;
  case 'T': while (++v <= last) v->x = (v->x >=F [v->i]) ? v->x : 0;       break;
  case '=': while (++v <= last) v->x = F [v->i];                           break;
  case '&': while (++v <= last) v->x = F [v->i] ? v->x : 0;                break;
  case '\\':
  case '!': while (++v <= last) v->x = F [v->i] ? 0 : v->x;                break;
  default: assert (0 && "unknown vector-full operation");
  }
  //chop_vec (V);
}

ix_t *vec_x_vec (ix_t *X, char op, ix_t *Y) {
  ixy_t *Z = join (X, Y, 0), *end = Z + len(Z), *z = Z-1;
  switch (op) {
  case '+': while (++z < end) z->x += z->y;                       break;
  case '-': while (++z < end) z->x -= z->y;                       break;
  case '.': while (++z < end) z->x *= z->y;                       break;
  case '*': while (++z < end) z->x *= z->y;                       break;
  case '/': while (++z < end) z->x /= z->y;                       break;
  case '^': while (++z < end) z->x = powf(z->x,z->y);             break;
  case '&': while (++z < end) z->x = z->y ? z->x : 0;             break;
  case '|': while (++z < end) z->x = z->x ? z->x : z->y;          break;
  case '\\':
  case '!': while (++z < end) z->x = z->y ? 0 : z->x;             break;
  case '<': while (++z < end) z->x = (z->x < z->y) ? 1 : 0;       break;
  case '>': while (++z < end) z->x = (z->x > z->y) ? 1 : 0;       break;
  case 'm': while (++z < end) z->x = (z->x < z->y) ? z->x : z->y; break;
  case 'M': while (++z < end) z->x = (z->x > z->y) ? z->x : z->y; break;
  case '=': while (++z < end) z->x = z->y ? z->y : z->x;          break;
  case 'H': while (++z < end) z->x = z->x * log (z->x / z->y);    break;
  case 'P': while (++z < end) z->x = z->x / MAX (1,(z->x+z->y));  break;
  default: assert (0 && "unknown vector-vector operation");
  }
  return ixy_to_ix_and_free (Z);
}

double bin_entropy (float p) {
  return - (p * log(p) + (1-p) * log(1-p)) / log(2);
}

void vec_x_num (ix_t *A, char op, double b) {
  ix_t *e = A+len(A), *a = A-1;
  switch (op) {
  case 'b': while (++a<e) a->x = (a->x != 0);              break;
  case 'l': while (++a<e) a->x = log (a->x);               break;
  case 'e': while (++a<e) a->x = exp (a->x);               break;
  case 'E': while (++a<e) a->x = erf (b * a->x);           break; // error func
  case 'A': while (++a<e) a->x = ABS (a->x);               break;
  case 'S': while (++a<e) a->x = SGN (a->x);               break;
  case 's': while (++a<e) a->x = 1.0/(1 + exp(-b * a->x)); break; // sigmoid
  case 'r': while (++a<e) a->x = exp (-b * ABS(a->x));   break; // RBF
  case '+': while (++a<e) a->x = a->x + b;                 break;
  case '-': while (++a<e) a->x = a->x - b;                 break;
  case '.': while (++a<e) a->x = a->x * b;                 break;
  case '*': while (++a<e) a->x = a->x * b;                 break;
  case '/': while (++a<e) a->x = a->x / (b?b:1);           break;
  case '<': while (++a<e) a->x = (a->x <  b) ? 1 : 0;      break;
  case '>': while (++a<e) a->x = (a->x >  b) ? 1 : 0;      break;
  case 'm': while (++a<e) a->x = (a->x <  b) ? a->x : b;   break;
  case 'M': while (++a<e) a->x = (a->x >  b) ? a->x : b;   break;
  case 't': while (++a<e) a->x = (a->x <= b) ? a->x : 0;   break;
  case 'T': while (++a<e) a->x = (a->x >= b) ? a->x : 0;   break;
  case '=': while (++a<e) a->x = b;                        break;
  case '^': while (++a<e) a->x = powf (a->x, b);           break;
  case 'p': while (++a<e) a->x = powa (a->x, b);           break;
  case '[': while (++a<e) a->x = floorf (a->x);            break;
  case ']': while (++a<e) a->x = ceilf (a->x);             break;
  case 'i': while (++a<e) a->x = roundf (a->x);            break;
  case 'H': while (++a<e) a->x = bin_entropy (a->x);       break;
  default: assert (0 && "unknown vector-scalar operation");
    //case 'r': for (; a<e; ++a) a->x = random()*1.0/RAND_MAX; break;
    //case '%': for (; a<e; ++a) a->x = random() < b/100;      break;
  }
  //chop_vec(A);
}

void num_x_vec (double b, char op, ix_t *A) {
  ix_t *e = A+len(A), *a = A-1;
  switch (op) {
  case '-': while (++a<e) a->x = b - a->x;              break;
  case '/': while (++a<e) a->x = b / a->x;              break;
  case '^': while (++a<e) a->x = powf (b, a->x);        break;
  case '<': while (++a<e) a->x = (b < a->x) ? 1 : 0;    break;
  case '>': while (++a<e) a->x = (b > a->x) ? 1 : 0;    break;
  default: vec_x_num (A, op, b);
  }
  //chop_vec(A);
}

void vec_x_range (ix_t *A, char op, xy_t R) { // [x,y)
  ix_t *a = A-1, *e = A + len(A);
  switch (op) {
  case '.':
  case '*': while (++a<e) a->x = (R.x <= a->x && a->x <= R.y) ? a->x : 0; break;
  case '-': while (++a<e) a->x = (a->x < R.x  ||  R.y < a->x) ? a->x : 0; break;
  case '=': while (++a<e) a->x = (a->x < R.x) ? R.x : (a->x > R.y) ? R.y : a->x; break;
  default: assert (0 && "unknown vector-range operation");
  }
  chop_vec(A);
}

ix_t *vec_X_vec (ix_t *A, ix_t *B) { // polynomial expansion
  ix_t *X = new_vec (0, sizeof(ix_t)), new = {0,0};
  ix_t *a=A-1, *b=B-1, *aEnd = A+len(A), *bEnd = B+len(B);
  while (++a < aEnd) while (++b < bEnd) {
      new.i = (a->i * (a->i - 1)) / 2 + b->i;
      new.x = a->x * b->x;
      X = append_vec (X, &new);
    }
  return X;
}

inline void f_vec (double (*F) (double), ix_t *A) {
  ix_t *e = A+len(A), *a = A-1;
  while (++a<e) a->x = F(a->x);
}


//////////////////////////////////////////////////
/// The following is used to convert points from
/// the linear scale to the gaussian scale.
/// Linear (0.5) is aligned with Gaussian (0)
///
/// The code is adopted from Mike Scudder's tdteval
/// (developed for TDT1)

#define A0      2.5066282388
#define A1    -18.6150006252
#define A2     41.3911977353
#define A3    -25.4410604963
#define B1     -8.4735109309
#define B2     23.0833674374
#define B3    -21.0622410182
#define B4      3.1308290983
#define C0     -2.7871893113
#define C1     -2.2979647913
#define C2      4.8501412713
#define C3      2.3212127685
#define D1      3.5438892476
#define D2      1.6370678189
#define LL      140

double ppndf (double p) {
  double q, r, retval;
  if (p >= 1.0) p = 1 - EPS;
  if (p <= 0.0) p = EPS;
  q = p - 0.5;
  if (ABS(q) <= 0.42) {
    r = q * q;
    retval = q * (((A3 * r + A2) * r + A1) * r + A0) /
      ((((B4 * r + B3) * r + B2) * r + B1) * r + 1.0);
  } else {
    //r = sqrt (log (0.5 - abs(q)));
    r = (q > 0.0  ?  1.0 - p : p);
    if (r <= 0.0) { fprintf (stderr, "Found r = %f\n", r); return r; }
    r = sqrt ((-1.0) * log (r));
    retval = (((C3 * r + C2) * r + C1) * r + C0) /
      ((D2 * r + D1) * r + 1.0);
    if (q < 0) retval *= -1.0;
  }
  return retval;
}
