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
#include <err.h>
#include <omp.h>
#include "matrix.h"
#include "textutil.h"
#include "svm.h"

//void mtx_reset_corrupt (char *C) { free_coll (open_coll (C,"a")); } // now in testvec

void mtx_copy (char *SRC, char *TRG) {
  coll_t *S = open_coll (SRC, "r+"), *T = open_coll (TRG, "w+");
  uint id, n = num_rows(S);
  for (id = 1; id <= n; ++id) {
    void   *vec = get_vec(S,id);
    if (len(vec)) put_vec(T,id,vec);
    if (!(id%10)) show_progress (id, n, "rows");
  }
  free_coll (S); free_coll (T);
}

void mtx_size (char *_M, char *prm) {
  coll_t *M = open_coll (_M,"r+");
  if      (strstr(prm,":r")) printf ("%u\n", num_rows(M));
  else if (strstr(prm,":c")) printf ("%u\n", num_cols(M));
  else printf ("%s: %u x %u\n", _M, num_rows(M), num_cols(M));
  free_coll (M);
}

void mtx_norm (char *_M, char *prm) {
  float p = getprm(prm,"p=",1);
  coll_t *M = open_coll (_M,"r+");
  printf ("%f\n", sum_sums(M,p));
  free_coll (M);
}

void mtx_trace (char *_M, char *prm) {
  double T = 0, N = 0;
  coll_t *M = open_coll (_M,"r+");
  uint i, nr = num_rows(M);
  for (i = 1; i <= nr; ++i) { // diagonal: row i, col i
    ix_t *row = get_vec_ro (M,i), *r = vec_find (row,i);
    if (r) T += r->x;
    if (len(row)) ++N;
  }
  if (strstr(prm,"avg")) T /= N?N:1;
  printf ("%f\n", T);
  free_coll (M);
}

void mtx_load (char *M, char *RH, char *CH, char *type, char *prm) {
  ulong done = 0;
  char *buf = malloc(1<<24), *id = 0;
  char *rcv = strstr(type,"rcv"), *svm = strstr(type,"svm");
  char *csv = strstr(type,"csv"), *txt = strstr(type,"txt"); 
  char *xml = strstr(type,"xml");
  char *BEG = xml ? "<DOC" : "", *END = xml ? "</DOC>" : "\n";
  assert (rcv || svm || csv || txt || xml);
  char *skip = strstr(prm,"skip"), *repl = strstr(prm,"repl");
  char *Long = strstr(prm,"long"), *join = strstr(prm,"join");
  char *p = getprms(prm,"p=","waa",',');
  char *pM = (p[0] == 'w') ? "w+" : (p[0] == 'a') ? "a+" : "r+";
  char *pR = (p[1] == 'w') ? "w"  : (p[1] == 'a') ? "a"  : "r";
  char *pC = (p[2] == 'w') ? "w"  : (p[2] == 'a') ? "a"  : "r";
  coll_t *m = open_coll (M, pM);
  RH = (RH && *RH && !atoi(RH)) ? RH : NULL;
  CH = (CH && *CH && !atoi(CH)) ? CH : NULL;
  hash_t *rh = !RH ? NULL : open_hash (RH, pR);
  hash_t *ch = !CH ? NULL : (rh && !strcmp(RH,CH)) ? rh : open_hash (CH, pC);
  fprintf (stderr, "[%.0fs] stdin --> %s [%s x %s] permissions: %s,%s,%s\n", vtime(), 
	   m->path, (rh ? rh->path : "numbers"), (ch ? ch->path : "numbers"),
	   m->access, (rh ? rh->access : "-"),  (ch ? ch->access : "-"));
  if (rcv) scan_mtx (m, NULL, rh, ch);
  else while (read_doc (stdin, buf, 1<<24, BEG, END)) {
      if (!xml && *buf == '#') continue; // skip comments (lines starting with '#')
      ix_t *vec = (xml ? parse_vec_xml (buf, &id, ch, prm) :
		   txt ? parse_vec_txt (buf, &id, ch, prm) :
		   svm ? parse_vec_svm (buf, &id, ch) :
		   csv ? parse_vec_csv (buf, &id) : NULL);
      if (!vec) continue;
      uint row = csv ? nvecs(m)+1 : rh ? key2id(rh,id) : (uint) atoi(id);
      /*
      if (!row) ; // no id (read-only rowhash)
      else if (!has_vec (m, row)) put_vec (m, row, vec);
      else if (repl) put_vec (m, row, vec);
      else if (skip) ;
      else if (join) {
	ix_t *old = get_vec (m,row), *tmp = vec;
	vec = vec_x_vec (vec, '+', old);
	free_vec (tmp); free_vec (old);
	put_vec (m, row, vec);
      }
      else fprintf (stderr, "\nWARNING: skipping duplicate: %s\n", id);
      */
      mtx_append (m, row, vec, (join?'+' : skip?'s' : repl?'r' : Long?'l' : 'r'));
      free (id);
      free_vec (vec);
      if (++done%100 == 0) show_progress (done, 0, "rows");
    }
  m->rdim = rh ? nkeys(rh) : num_rows (m);
  m->cdim = ch ? nkeys(ch) : num_cols (m);
  //if (symlink (RH, cat(M,"/hash.rows"))) perror (RH);
  //if (symlink (CH, cat(M,"/hash.cols"))) perror (CH);
  fprintf (stderr, "[%.0fs] %d rows, %d cols\n", vtime(), num_rows(m), num_cols(m)); 
  free_coll (m); 
  free_hash (rh);
  if (ch!=rh) free_hash (ch);
  if (p) free(p);
  free (buf);
}

void mtx_print (char *prm, char *_M, char *RH, char *CH) {
  if (!prm) prm = "";
  uint top = getprm (prm,"top=",0), rno = getprm (prm,"rno=",0);
  //uint dd = getprm (prm,"dd=",4);
  char *rcv = strstr(prm,"rcv"), *txt = strstr(prm,"txt"), *xml = strstr(prm,"xml");
  char *svm = strstr(prm,"svm"), *csv = strstr(prm,"csv"), *ids = strstr(prm,"ids");
  char *jsn = strstr(prm,"json");
  char *fmt = getprms (prm,"fmt=",NULL,',');
  char *rid = getprms (prm,"rid=",NULL,',');
  char *empty = strstr(prm,"empty");
  coll_t *M = open_coll (_M, "r+");
  RH = (RH && *RH && !atoi(RH)) ? RH : NULL;
  CH = (CH && *CH && !atoi(CH)) ? CH : NULL;
  hash_t *rh = !RH ? NULL : open_hash (RH, "r");
  hash_t *ch = !CH ? NULL : (rh && !strcmp(RH,CH)) ? rh : open_hash (CH, "r");
  uint i, nr = num_rows (M), nc = num_cols (M);
  fprintf (stderr, "# %s [%dx%d]\n", _M, nr, nc);
  uint beg_i = rno ? rno : rid ? key2id (rh,rid) : 1;
  uint end_i = (rno || rid) ? beg_i : nr;
  for (i = beg_i; i <= end_i; ++i) {
    ix_t *vec = get_vec (M, i);
    if (!len(vec) && !empty) { free_vec(vec); continue; }
    char *rid = id2str(rh,i);
    if      (top) { trim_vec (vec, top); sort_vec (vec, cmp_ix_X); }
    if      (ids) printf ("%s\n", rid); 
    else if (rcv) print_vec_rcv (vec, ch, rid, fmt);
    else if (txt) print_vec_txt (vec, ch, rid, 0);
    else if (xml) print_vec_txt (vec, ch, rid, 1);
    else if (csv) print_vec_csv (vec, nc, fmt);
    else if (svm) print_vec_svm (vec, ch, rid, fmt);
    else if (jsn) print_vec_json(vec, ch, rid, fmt);
    else          print_vec_rcv (vec, ch, rid, fmt);
    free_vec(vec); free(rid);
  }
  free_coll (M); free_hash (rh); if (ch != rh) free_hash (ch);
  if (fmt) free(fmt);
}  

static uint *hash_merge_self (char *_A, char *_B) {
  assert (!strcmp(_A,_B));
  hash_t *B = open_hash (_B, "r");
  uint i, nB = nvecs(B->keys);
  uint *F = new_vec (nB+1, sizeof(uint));
  for (i = 1; i <= nB; ++i) F[i] = i;
  fprintf (stderr, "done: %s == %s [%d] \n", _A, _B, nB);
  free_hash (B);
  return F;
} 

// return an injection F: B -> A, insert missing keys into A
uint *hash_merge (char *_A, char *_B, char *pA) {
  if (!strcmp(_A,_B)) return hash_merge_self (_A, _B);
  hash_t *A = open_hash (_A, pA);
  hash_t *B = open_hash (_B, "r");
  uint i, nB = nvecs(B->keys), nA = nvecs(A->keys);
  uint *F = new_vec (nB+1, sizeof(uint));
  fprintf (stderr, "merge: %s [%d] += %s [%d] mode:%s\n", _A, nA, _B, nB, pA);
  for (i = 1; i <= nB; ++i) { // for each key in the table
    char *key = id2key (B,i);
    F[i] = key2id (A,key);
    if (0 == i%100) show_progress (i, nB, "keys merged");
  }
  fprintf (stderr, "done: %s [%d] \n", _A, nvecs(A->keys));
  free_hash (A); free_hash (B);
  return F;
}

void *shift_vec (void *V) {
  uint N = len(V), sz = vesize(V);
  V = resize_vec (V, N+1);
  memmove (V + sz, V, N*sz);
  return V;
}

// return an injection F: B -> A, insert missing keys into A
uint *hash_merge2 (char *_A, char *_B, char *pA) {
  if (!strcmp(_A,_B)) return hash_merge_self (_A, _B);
  char **keys = hash_keys (_B);
  hash_t *A = open_hash (_A, pA);
  uint nB = len(keys), nA = nvecs(A->keys);
  fprintf (stderr, "merge: %s [%d] += %s [%d] mode:%s\n", _A, nA, _B, nB, pA);
  uint *F = keys2ids (A, keys); 
  F = shift_vec (F);
  fprintf (stderr, "done: %s [%d] \n", _A, nvecs(A->keys));
  free_hash (A); free_toks (keys);
  return F;
}

void *get_vec_read (coll_t *c, uint id) ;
void put_vec_write (coll_t *c, uint id, void *vec) ;

// merge B [Rb x Cb] into A [Ra x Ca] mapping rows and columns as needed
void mtx_merge (char *_A, char *_RA, char *_CA,
		char *_B, char *_RB, char *_CB, 		
		char *prm) {
  char *skip = strstr(prm,"skip"), *repl = strstr(prm,"repl");
  char *Long = strstr(prm,"long"), *join = strstr(prm,"join");
  char *fast = strstr(prm,"fast");
  char *perm = getprms(prm,"p=","aaa",','); // access to A, Ra and Ca
  char *pA = (perm[0] == 'r') ? "r+" : (perm[0] == 'w') ? "w+" : "a+"; 
  char *pR = (perm[1] == 'r') ? "r"  : (perm[1] == 'w') ? "w"  : "a";
  char *pC = (perm[2] == 'r') ? "r"  : (perm[2] == 'w') ? "w"  : "a";
  uint *R = (fast ? hash_merge2 : hash_merge) (_RA, _RB, pR);
  uint *C = (fast ? hash_merge2 : hash_merge) (_CA, _CB, pC);
  coll_t *A = open_coll (_A, pA);
  coll_t *B = open_coll (_B, "r+");
  uint nB = num_rows(B), nA = num_rows(A), b;
  fprintf (stderr, "merge: %s [%d x %d] += %s [%d x %d] mode:%s\n", _A, nA, num_cols(A), _B, nB, num_cols(B), pA);
  for (b = 1; b <= nB; ++b) {
    uint a = R[b]; // map row: B[b] -> A[a]
    if (!a) continue;  // no target row => drop
    if (!has_vec (B,b)) continue;
    ix_t *V = get_vec (B,b), *v, *last = V+len(V)-1;
    assert (b < len(R) && last->i < len(C));
    for (v = V; v <= last; ++v) v->i = C[v->i]; // map column
    sort_vec (V, cmp_ix_i);
    chop_vec (V);
    if (len(V)) mtx_append (A, a, V, (join?'+' : skip?'s' : repl?'r' : Long?'l' : 'r'));
    /*  
    if (!has_vec (A,a)) put_vec_write (A,a,V); // no conflict
    else if (repl) put_vec_write (A,a,V); // replace A[a] with B[b]
    else if (Long && len(V) > len_vec (A,a)) put_vec_write (A,a,V);
    else {} // skip by default: keep A[a], drop B[b]
    */
    free_vec (V);
    show_progress (b, nB, "rows merged");
  }
  fprintf (stderr, "done: %s [%d x %d]\n", _A, num_rows(A), num_cols(A));
  free_coll (A); free_coll (B); free_vec (R); free_vec (C);
}

void chop_jix (jix_t *jix) ;
void append_jix (coll_t *c, jix_t *jix) ;
static void mtx_weigh_max (coll_t *trg, coll_t *src, char *prm) {
  char *low = strstr(prm,"min"), *row = strstr(prm,"row");
  jix_t *M = (row ? (low ? min_rows(src) : max_rows (src)) 
	      :     (low ? min_cols(src) : max_cols (src)));
  chop_jix (M);
  sort_vec (M, cmp_jix);
  append_jix (trg, M);
  free_vec (M);
  trg->rdim = src->rdim;
  trg->cdim = src->cdim;
}

static void mtx_weigh_sum (coll_t *trg, coll_t *src, char *prm) {
  char *col = strstr(prm,"colsum");
  float p = getprm(prm,"sum,p=",1);
  float *F = col ? sum_cols (src,p) : sum_rows (src,p);
  ix_t *vec = full2vec (F);
  put_vec (trg,1,vec);
  trg->rdim = 1;
  trg->cdim = col ? src->cdim : src->rdim;
  free_vec (F); free_vec (vec);
}

static it_t parse_slice (char *slice, uint min, uint max) ;
static float *mtx_feature_select (coll_t *src, char *prm) {
  char *p;  
  if ((p=strstr(prm,"FS:df="))) {
    it_t range = parse_slice (p+6, 1, num_rows(src));
    fprintf (stderr, "Feature selection: %d < df < %d ...", range.i-1, range.t+1);
    float *F = sum_cols (src, 0), *f;
    for (f = F; f < F+len(F); ++f) if (*f < range.i || *f > range.t) *f = 0;
    fprintf (stderr, "\n");
    return F;
  }
  return NULL;
}

static void mtx2full (coll_t *trg, coll_t *src) {
  uint i, nr = num_rows(src), nc = num_cols(src);
  fprintf (stderr, "%s [%d x %d] mtx -> full %s\n", src->path, nr, nc, trg->path);
  for (i=0; i<=nr; ++i) {
    ix_t *vec = get_vec (src, i);
    float *full = vec2full (vec, nc, 0);
    put_vec (trg, i, full);
    free_vec (vec); free_vec (full);
    show_progress (i,nr,"vecs");
  }
  trg->rdim = nr; trg->cdim = nc;
}

static void full2mtx (coll_t *trg, coll_t *src) {
  uint i, nr = num_rows(src), nc = num_cols(src);
  fprintf (stderr, "%s [%d x %d] full -> mtx %s\n", src->path, nr, nc, trg->path);
  for (i=0; i<=nr; ++i) {
    float *full = get_vec (src, i);
    ix_t *vec = full2vec (full);
    put_vec (trg, i, vec);
    free_vec (vec); free_vec (full);
    show_progress (i,nr,"vecs");
  }
  trg->rdim = nr; trg->cdim = nc;
}

void ptrim (ix_t *vec, uint n, float p) ;

void mtx_weigh (char *TRG, char *prm, char *SRC, char *STATS) { // thread-unsafe: doc2lm
  assert (TRG && SRC);
  char *flr = strstr(prm,"floor"), *cei = strstr(prm,"ceil");
  char *rou = strstr(prm,"round"), *chop = strstr(prm,"chop");
  char *lse = strstr(prm,"logSexp"), *l2p = strstr(prm,"log2p");
  char *smx = strstr(prm,"softmax"), *rnk = strstr(prm,"ranks");
  char *R01 = strstr(prm,"range01"), *r01 = strstr(prm,"row01");
  char *Sgn = strstr(prm,"sgn"), *Abs = strstr(prm,"abs");
  char *Sum = strstr(prm,"sum"), *Sm2 = strstr(prm,"sum2"), *Sm0 = strstr(prm,"sum0");
  char *Min = strstr(prm,"min"), *Max = strstr(prm,"max");
  char *Exp = strstr(prm,"exp"), *Log = strstr(prm,"log");
  char *inq = strstr(prm,"inq"), *idf = strstr(prm,"idf");
  char *std = strstr(prm,"std"), *uni = strstr(prm,"uni");
  char *L1  = strstr(prm,"L1"),  *L2  = strstr(prm,"L2");
  char *Lap = strstr(prm,"Lap");
  char *rnd = strstr(prm,"rnd"), *lsh = strstr(prm,"lsh");
  char *smh = strstr(prm,"simhash"), *smd = getprmp(prm,"simhash:","U");
  char *lm  = strstr(prm,"lm:"), *bm25= strstr(prm,"bm25");
  char *ntf = strstr(prm,"ntf");
  char *Cla = strstr(prm,"clarity"), *LM = strstr(prm,"LM:");
  char *Erf = strstr(prm,"erf");
  char *Acos = strstr(prm,"acos"), *Atan = strstr(prm,"atan");
  //uint top2 = getprm(prm,"top2=",0);
  float out = getprm(prm,"out=",0);
  float thr = getprm(prm,"thresh=",0); //, trp = getprm(prm,"ptrim=",0);
  float top = getprm(prm,"top=",0),  L = getprm(prm,"L=",0);
  float   k = getprm(prm,  "k=",0),  b = getprm(prm,"b=",0);
  float rbf = getprm(prm,"rbf=",0),  sig = getprm(prm,"sig=",0);
  
  //float lmj = getprm(prm,"lm:j=",0), lmd = getprm(prm,"lm:d=",0);
  //fprintf (stderr, "%s -> %f\n", prm, pow);
  if (l2p || lse) Log = 0; // 'logSexp' and 'log2p' contain 'log'
  if (lse) Exp = 0; // 'logSexp' contains 'exp'
  if (Sm2) Sum = 0; // 'sum2' contains 'sum'
  if (Sm0) Sum = 0; // 'sum0' contains 'sum'
  if (smx) Max = 0; // 'softmax' contains 'max'
  
  stats_t *stats = NULL; 
  if (inq || idf || bm25 || ntf || lm || std || out || Lap || Cla || LM) {
    fprintf (stderr, "gathering statistics from %s\n", (STATS?STATS:SRC));
    if (STATS && index(STATS,':')) { // file:dict
      char *_dict = index(STATS,':'); *_dict++ = 0;
      hash_t *dict = open_hash (_dict, "r");
      stats = blank_stats (1, 1, 0, 1) ;
      update_stats_from_file (stats, dict, STATS) ;
      free_hash (dict);
    } else {
      coll_t *c = open_coll ((STATS?STATS:SRC), "r+");
      stats = coll_stats (c);
      free_coll (c);
    }
    stats->k = k;
    stats->b = b;
    fprintf (stderr, "\n");
    //dump_stats (stats, NULL);
    //printf ("nwords: %d\n", stats->nwords);
    //printf ("ndocs:  %d\n", stats->ndocs);
    //printf ("nposts: %f\n", stats->nposts);
    //printf ("k: %f\n", stats->k);
    //printf ("b: %f\n", stats->b);
  }
  
  //uint ndocs = num_rows (stats);
  //uint  *df = (inq || idf || bm25) ? len_cols (stats) : NULL;
  //float *s1 = (std || lmj || lmd) ? sum_cols (stats, 1) : NULL;
  //float *s2 = std ? sum_cols (stats, 2) : NULL;
  //double nposts = (inq || bm25 || lmj || lmd) ? sum_sums (stats, 1) : 0;
  
  //uint copy = strcmp (SRC,TRG); // copy or in-place?
  coll_t *src = open_coll (SRC,"r+");// : open_coll (TRG,"a+");
  coll_t *trg = open_coll (TRG,"w+");// : src;
  
  trg->rdim = src->rdim;
  trg->cdim = (Max || Min || Sm0 || Sm2 || Sum || lse) ? 1 : smh ? (L*k) : src->cdim;
  
  xy_t range = R01 ? mrange (src) : (xy_t){0,0};
  float *FS = mtx_feature_select (src, prm);
  
  uint id = 0, nv = num_rows (src);
  if      (strstr(prm,"mtx2full")) { mtx2full (trg,src);          id = nv; }
  else if (strstr(prm,"full2mtx")) { full2mtx (trg,src);          id = nv; }
  else if (strstr(prm,"softcol"))  { col_softmax (trg,src);       id = nv; }
  else if (strstr(prm,"colsum") ||
	   strstr(prm,"rowsum"))   { mtx_weigh_sum (trg,src,prm); id = nv; }
  else if (strstr(prm,"colmax") || 
	   strstr(prm,"colmin") || 
	   strstr(prm,"rowmax") || 
	   strstr(prm,"rowmin"))   { mtx_weigh_max (trg,src,prm); id = nv; }
  
  while (++id <= nv) {
    show_progress (id, nv, "vecs");
    if (!has_vec (src, id)) continue;
    ix_t *vec = get_vec (src, id), *tmp = 0; // *e = vec + len(vec), *v = vec-1;
    if (!len(vec)) { free_vec (vec); continue; }
    if     (chop) chop_vec (vec);
    if      (inq) weigh_vec_inq (vec, stats);
    else if (idf) weigh_vec_idf (vec, stats);
    else if (ntf) weigh_vec_ntf (vec, stats);
    else if(bm25) weigh_vec_bm25(vec, stats);
    else if (lm && b) weigh_vec_lmj (vec, stats);
    else if (lm && k) weigh_vec_lmd (vec, stats);
    else if (Cla) weigh_vec_clarity (vec, stats);
    else if (LM && k) { vec = doc2lm (tmp=vec, stats->cf, k, 0); free_vec(tmp); }
    else if (LM && b) { vec = doc2lm (tmp=vec, stats->cf, 0, b); free_vec(tmp); }
    else if (out) crop_outliers (vec, stats, out);
    else if (std) weigh_vec_std (vec, stats);
    else if (Lap) weigh_vec_laplacian (vec, stats);
    else if (rnd) weigh_vec_rnd (vec);
    else if (rbf) vec_x_num (vec, 'r', rbf); // radial basis function
    else if (sig) vec_x_num (vec, 's', sig); // sigmoid (logistic)
    else if (Sgn) vec_x_num (vec, 'S', 1); // sign
    else if (Abs) vec_x_num (vec, 'A', 1); // absolute value
    else if (Erf)  f_vec (erf,  vec); 
    else if (Log)  f_vec (log,  vec); 
    else if (Exp)  f_vec (exp,  vec); 
    else if (Acos) f_vec (acos, vec); 
    else if (Atan) f_vec (atan, vec);
    if      (FS) { vec_x_full (vec, '&', FS); chop_vec (vec); }
    if      (top) trim_vec (vec, top);
    if      (thr) vec_x_num (vec, 'T', thr);
    //if     (top2) trim_vec2 (vec, top2);
    if      (l2p) softmax (vec); // log2posterior
    else if (smx) softmax (vec); // log2posterior
    else if (rnk) weigh_vec_ranks (vec);
    else if (uni) vec_x_num (vec, '=', 1./len(vec));
    else if  (L1) vec_x_num (vec, '/', sum (vec)); 
    else if  (L2) vec_x_num (vec, '/', sqrt (sum2 (vec))); 
    else if (R01) { vec_x_num (vec, '-', range.x); vec_x_num (vec, '/', range.y-range.x); }
    else if (r01) weigh_vec_range01 (vec);
    else if (flr) vec_x_num (vec, '[', 0);
    else if (cei) vec_x_num (vec, ']', 0);
    else if (rou) vec_x_num (vec, 'i', 0);
    if      (smh) { vec = simhash (tmp=vec, L*k, smd); free_vec (tmp); }
    if      (lsh) { vec = bits2codes (tmp=vec, L);     free_vec (tmp); }
    if      (Max) { vec->i=1; vec->x = max(vec)->x;      len(vec)=1; }
    else if (Min) { vec->i=1; vec->x = min(vec)->x;      len(vec)=1; }
    else if (Sm0) { vec->i=1; vec->x = sump(0,vec);      len(vec)=1; }
    else if (Sm2) { vec->i=1; vec->x = sum2(vec);        len(vec)=1; }
    else if (Sum) { vec->i=1; vec->x = sum(vec);         len(vec)=1; }
    else if (lse) { vec->i=1; vec->x = log_sum_exp(vec); len(vec)=1; }
    put_vec (trg, id, vec);
    free_vec (vec);
  }
  
  free_stats (stats); free_vec (FS);
  free_coll (src); 
  free_coll (trg);
  //if (copy) free_coll (trg);
}

void mtx_transpose (char *prm, char *src, char *trg) {
  char buf[9999];
  coll_t *M = open_coll (src, "r+");
  if (!trg) trg = fmt(buf,"%s.T",src);
  coll_t *T = open_coll (trg, "w+");
  float MB = M->offs[0] / (1<<20), ETA = MB/1400;
  fprintf (stderr, "transposing %s %.0fMB, should be done in %.1f minutes\n", src, MB, ETA);
  transpose_mtx (M,T); // rdim/cdim updated here
  free_coll (T);
  free_coll (M);
  (void) prm;
}

inline void dot2cosine (ix_t *dot, float A2, float *B2) {
  ix_t *d = dot-1, *end = dot + len(dot);
  while (++d < end) d->x /= sqrt (A2 * B2[d->i]); 
}

inline void dot2euclid (ix_t *dot, float A2, float *B2) {
  ix_t *d = dot-1, *end = dot + len(dot); // negative Euclidian
  while (++d < end) d->x = -sqrt (A2 + B2[d->i] - 2 * d->x); 
}

inline void dot2dice (ix_t *dot, float A2, float *B2) {
  ix_t *d = dot-1, *end = dot + len(dot); 
  while (++d < end) d->x = 2 * d->x / (A2 + B2[d->i]);
}

inline void dot2jaccard (ix_t *dot, float A2, float *B2) {
  ix_t *d = dot-1, *end = dot + len(dot); 
  while (++d < end) d->x = d->x / (A2 + B2[d->i] - d->x);
}

inline void dot2lmd (ix_t *dot, float QL, float *DL, float mu) {
  ix_t *d = dot-1, *end = dot + len(dot);
  while (++d < end) d->x += QL * log (mu / (mu + DL[d->i]));
}

void dot2lm (coll_t *P, coll_t *Q, char *prm) { // products => LM scores
  fprintf (stderr, "[%.0fs] products -> LM scores using %s\n", vtime(), prm);
  
  char *docs = getprms (prm,"docs=",NULL,',');
  coll_t *D = open_coll (docs, "r+");
  float *DL = sum_rows (D, 1); // document length
  float *CF = sum_cols (D, 1); // collection frequency
  double nposts = sumf (DL); // collection length
  free_coll (D);
  if (docs) free (docs);
  
  float mu = getprm (prm,"lm:d=",0), lambda = getprm (prm,"lm:j=",0);
  uint id, n = num_rows (P);
  for (id = 1; id <= n; ++id) {
    ix_t *qry = get_vec (Q, id), *q = qry-1, *qend = qry+len(qry);
    ix_t *ret = get_vec (P, id), *r = ret-1, *rend = ret+len(ret);
    double ql = sum(qry), logBG = 0;
    while (++q < qend) logBG += q->x * log (CF[q->i] / nposts);
    if (mu) while (++r < rend) r->x += logBG + ql * log (mu / (mu + DL[r->i]));
    else    while (++r < rend) r->x += logBG + ql * log (1 - lambda);
    put_vec (P, id, ret);
    free_vec(qry); free_vec(ret);
  }
  free_vec (DL); free_vec (CF); 
}

void mtx_subset (char *_A, char *_B, char *_H) {
  char ID[1000], *NL; uint done = 0;
  fprintf (stderr, "%s = subset %s [%s] reading ids from stdin\n", _A, _B, _H);
  coll_t *A = open_coll (_A, "w+");
  coll_t *B = open_coll (_B, "r+");
  hash_t *H = open_hash (_H, "r+");
  while (fgets(ID,999,stdin)) {
    if ((NL = strchr (ID,'\n'))) *NL = 0;
    uint id = key2id (H,ID);
    if (!id || !has_vec (B,id)) { fprintf (stderr, "WARNING: no vec for %s(%d)\n", ID, id); continue; }
    ix_t *vec = get_vec (B,id);
    put_vec (A,id,vec);
    free_vec (vec);
    show_progress (++done, 0, "vecs copied");
  }
  free_coll (A); free_coll (B); free_hash (H);
}

void mtx_product (char *_P, char *_A, char *_B, char *prm) {
  assert (_P && _A && _B && strcmp(_P,_A) && strcmp(_P,_B));
  if (!prm) prm = ""; 
  char sim = (strstr(prm,"eucl") ? '2' : strstr(prm,"manh") ? '1' : 
	      strstr(prm,"hamm") ? '0' : strstr(prm,"cos")  ? 'C' : 
	      strstr(prm,"jacc") ? 'J' : strstr(prm,"hell") ? 'H' : 
	      strstr(prm,"dice") ? 'D' : strstr(prm,"chi2") ? 'X' : 
	      strstr(prm,"norm") ? 'n' : strstr(prm,"NORM") ? 'N' : 
	      strstr(prm,"BAND") ? 'B' : strstr(prm,"slin") ? 'k' : 
	      strstr(prm,"smax") ? 'S' : strstr(prm,"smin") ? 's' : 	      
	      strstr(prm,"maxx") ? 'M' : strstr(prm,"minx") ? 'm' : '.');
  float p = index("2CJD",sim) ? 2 : index("1HX",sim) ? 1 : getprm(prm,",p=",0);
  int keepz = strstr(prm,"keepzero") || index("012nN",sim); // keep 0 distance
  int  top = getprm (prm,"top=",0);
  float thresh = getprm (prm,"thresh=",0);
  //float rbf = getprm (prm,"rbf=",0);
  uint threads = getprm (prm,"threads=",0), cache = getprm (prm,"cache=",64);
  coll_t *P = open_coll (_P, "w+");
  coll_t *A = open_coll (_A, "r+");
  MAP_SIZE = ((off_t) cache) << 20;
  coll_t *B = open_coll (_B, "r+");
  P->rdim = A->rdim;
  P->cdim = B->cdim;
  if (A->cdim != B->rdim) warnx("WARNING: incompatible dimensions %s [%d x %d], %s [%d x %d]", _A, A->rdim, A->cdim, _B, B->rdim, B->cdim);
  float *SA = (sim == '.') ? NULL : norm_rows(A,p,0);
  float *SB = (sim == '.') ? NULL : norm_cols(B,p,0);
  uint id, j, nA = num_rows (A), nB = SB ? (len(SB)-1) : num_cols(B), done=0;
  fprintf (stderr, "[%.0fs] computing %s [%dx%d]: %.0fM similarities(%c), p=%.2f, top=%d, cache=%dM, %d threads\n",
	   vtime(), _P, nA, nB, (nA*nB/1E6), sim, p, top, cache, threads);
  //#pragma omp parallel for private(id) schedule(dynamic) if (threads) num_threads(threads)
  for (id = 1; id <= nA; ++id) {
    float *S = new_vec (nB+1,sizeof(float));
    //ix_t *_a = get_vec (A, id), *a = _a-1, *aEnd = _a+len(_a); // don't use get_vec_ro
    ix_t *_a = get_vec_mp (A, id), *a = _a-1, *aEnd = _a+len(_a);
    if (!len(_a)) { free_vec(_a); free_vec(S); continue; }
    while (++a < aEnd) {
      //ix_t *_b = get_vec_ro (B, a->i), *b = _b-1, *bEnd = _b+len(_b);
      ix_t *_b = get_vec_mp (B, a->i), *b = _b-1, *bEnd = _b+len(_b);
      switch (sim) {
      case 'H': while (++b<bEnd) S[b->i] += sqrt ((a->x / SA[id]) * (b->x / SB[b->i]));      break;
      case 'X': while (++b<bEnd) S[b->i] +=  2 / ((SA[id] / a->x) + (SB[b->i] / b->x));      break;
      case 'n':
      case 'N': while (++b<bEnd) S[b->i] += powa(a->x-b->x,p) - powa(a->x,p) - powa(b->x,p); break;
      case '0': while (++b<bEnd) S[b->i] +=    (a->x != b->x) -    (a->x!=0) -    (b->x!=0); break;
      case '1': while (++b<bEnd) S[b->i] +=    ABS(a->x-b->x) -    ABS(a->x) -    ABS(b->x); break;
      case '2':
      case 'C':
      case 'D':
      case 'J':
      case '.': while (++b<bEnd) S[b->i] += a->x * b->x;                                     break;
      case 'B': while (++b<bEnd) S[b->i] += (a->x > 0) && (b->x > 0);                        break;
      case 'k': while (++b<bEnd) S[b->i] = MAX (S[b->i], MIN (a->x, b->x));                  break;
      case 's': while (++b<bEnd) S[b->i] += MIN (a->x, b->x);                                break;
      case 'S': while (++b<bEnd) S[b->i] += MAX (a->x, b->x);                                break;
      case 'M': while (++b<bEnd) S[b->i] = MAX (S[b->i], (a->x * b->x));                     break;
      case 'm': while (++b<bEnd) S[b->i] = MIN (S[b->i], (a->x * b->x));                     break;
      }
      free_vec (_b);
    }
    switch (sim) {
    case 'C': for (j=1;j<=nB;++j) if (S[j]) S[j] /=    sqrt (SA[id] * SB[j]); break;
    case 'D': for (j=1;j<=nB;++j) if (S[j]) S[j] /=   0.5 * (SA[id] + SB[j]); break;
    case 'J': for (j=1;j<=nB;++j) if (S[j]) S[j] /= (-S[j] + SA[id] + SB[j]); break;
    case 'H': for (j=1;j<=nB;++j) S[j] = 1 - sqrt (ABS(1 - S[j]));            break;
    //case 'X': for (j=1;j<=nB;++j) S[j] = -(1 - S[j]);                         break;
    case '0': 
    case '1': 
    case 'N': for (j=1;j<=nB;++j) S[j] =     -(SA[id] + SB[j] +   S[j]);      break;
    case 'n': for (j=1;j<=nB;++j) S[j] = -powa(SA[id] + SB[j] +   S[j], 1/p); break;
    case '2': for (j=1;j<=nB;++j) S[j] = -sqrt(SA[id] + SB[j] - 2*S[j]);      break;
    case 'B': for (j=1;j<=nB;++j) S[j] = (S[j] == len(_a));                   break;
    case 'k': for (a=_a;a<aEnd;++a) S[a->i] = MAX (S[a->i], a->x);            break;
    }
    ix_t *_c = keepz ? full2vec_keepzero(S) : full2vec(S); 
    //if (rbf) vec_x_num (_c, 'r', rbf);
    if (top) trim_vec (_c, top);
    if (thresh) vec_x_num (_c, 'T', thresh);
    if (!keepz) chop_vec (_c);
    //#pragma omp critical
    put_vec (P, id, _c);
    free_vec (_a); 
    free_vec (_c); 
    free_vec (S);
    if (omp_get_thread_num() == 0) show_progress (++done, nA, "rows");
  }
  free_coll (B);
  free_vec (SA);
  free_vec (SB);
  if (strstr (prm,"lm:")) dot2lm (P, A, prm);
  free_coll (P); 
  free_coll (A); 
  fprintf (stderr, "[%.0fs] %d rows done\n", vtime(), nA);
}

inline ix_t *fill_mask (float *full, ix_t *mask) {
  assert (last_id(mask) < len(full));
  //ix_t *m = mask-1, *end = mask+len(mask);
  //while (++m < end) m->x = full [m->i];
  ix_t *out = copy_vec (mask), *o = out-1, *end = out+len(out);
  while (++o < end) o->x = full [o->i];
  return out;
}

void mtx_distance (char *_D, char *_A, char *_B, char *prm) {
  assert (_D && _A && _B && strcmp(_D,_A) && strcmp(_D,_B));
  if (!prm) prm = ""; 
  char *cos = strstr(prm,"cos"), *cor = strstr(prm,"corr"), *R2 = strstr(prm,"R2");
  char *chi = strstr(prm,"chi2"), *lse = strstr(prm,"logSexp");
  char *Min = strstr(prm,"min"), *Max = strstr(prm,"max");
  uint kdt = getprm (prm,"kdot=",0);
  char *sim = chi ? "Chi^2" : lse ? "logSexp" : Min ? "min" : Max ? "max" : kdt ? "k-dot" : "p-norm";
  char *wts = getprms (prm,"wts=",NULL,',');  ix_t *WTS = NULL;
  if (wts) { coll_t *W = open_coll (wts, "r+"); WTS = get_vec (W,1); free_coll (W); }
  int mask = lse || Min || Max;
  int top = getprm (prm,"top=",0);
  float p = getprm(prm,":p=",getprm(prm,",p=",0));
  float rbf = getprm(prm,"rbf=",0);
  coll_t *D = open_coll (_D, "w+");
  coll_t *A = open_coll (_A, "r+");
  coll_t *B = open_coll (_B, "r+");
  D->rdim = A->rdim;
  D->cdim = B->rdim;
  if (A->cdim != B->cdim) warnx("WARNING: incompatible dimensions %s [%d x %d], %s [%d x %d]", _A, A->rdim, A->cdim, _B, B->rdim, B->cdim);
  uint i, j, nA = num_rows(A), nB = num_rows(B), nC = num_cols(B);
  ulong done = 0;
  fprintf (stderr, "[%.0fs] computing %s [%dx%d]: %.0fM %s distances, p=%.2f, k=%d, top=%d\n", 
	   vtime(), _D, nA, nB, (nA*nB/1E6), sim, p, kdt, top);
  for (j = 1; j <= nA; ++j) {
    //if (!has_vec (A,j)) continue; // incorrect: zero vec => nonzero distances
    ix_t *d = const_vec (nB,0);
    ix_t *a = get_vec (A,j);
    float *aa = mask ? vec2full (a,nC,0) : NULL;
    for (i = 1; i <= nB; ++i) {
      ix_t *b = get_vec_ro (B,i);
      if (mask) {
	b = fill_mask (aa,b); // creates a copy
	if      (Max) d[i-1].x = max(b)->x;
	else if (Min) d[i-1].x = min(b)->x;
	else if (lse) d[i-1].x = log_sum_exp(b); 
	free_vec (b);
      }
      else if (kdt) d[i-1].x = kdot (kdt, a, b);
      else if (chi) d[i-1].x = -chi2 (a, b);
      else if (cos) d[i-1].x = cosine (a, b);
      else if (cor) d[i-1].x = correlation (a, b, nC);
      else if (R2)  d[i-1].x = powa (correlation (a, b, nC), 2); //rsquared (a, b, nC);
      else if (WTS) d[i-1].x = -wpnorm (p, a, b, WTS);
      else          d[i-1].x = -pnorm (p, a, b);
      if      (rbf) d[i-1].x = exp (rbf * d[i-1].x);
    }
    if (top) trim_vec (d, top);
    put_vec (D, j, d);
    free_vec (d);
    free_vec (a);
    free_vec (aa);
    show_progress ((done+=nB), 0, "distances");
  }
  free_coll (D); free_coll (A); free_coll (B);
  fprintf (stderr, "[%.0fs] %ld cells done\n", vtime(), done);
}

void mtx_dot (char *_P, char *_A, char op, char *_B) {
  //char *chop = strstr(prm,"chop");
  assert (_P && _A && _B && strcmp(_P,_A) && strcmp(_P,_B));
  float xA = strtof(_A,0), xB = strtof(_B,0);
  coll_t *A = coll_exists (_A) ? open_coll (_A, "r+") : NULL;
  coll_t *B = coll_exists (_B) ? open_coll (_B, "r+") : NULL;
  coll_t *P =                    open_coll (_P, "w+");
  uint nA = A ? num_rows (A) : 0;
  uint nB = B ? num_rows (B) : 0;
  P->rdim = A ? A->rdim : B ? B->rdim : 0;
  P->cdim = A ? A->cdim : B ? B->cdim : 0;
  if (A && B && (A->rdim != B->rdim || A->cdim != B->cdim)) warnx("WARNING: incompatible dimensions %s [%d x %d], %s [%d x %d]", _A, A->rdim, A->cdim, _B, B->rdim, B->cdim);
  uint id, n = MAX(nA,nB);
  for (id = 1; id <= n; ++id) {
    ix_t *a = A ? get_vec (A, id) : NULL;
    ix_t *b = B ? get_vec (B, id) : NULL;
    ix_t *p = NULL;
    if (a && b) p = vec_x_vec (a, op, b);
    else if (a) { vec_x_num (a, op, xB); p = copy_vec(a); }
    else if (b) { num_x_vec (xA, op, b); p = copy_vec(b); }
    else assert (0 && "matrix operation on two scalars");
    chop_vec (p);
    put_vec (P, id, p);
    free_vec (a); free_vec (b); free_vec (p);
    show_progress (id, n, "rows");
  }
  free_coll (P); free_coll (A); free_coll (B);
}

void mtx_add (char *_P, char *wA, char *_A, char op, char *wB, char *_B) {
  assert (_P && _A && _B && wA && wB && (op == '+' || op == '-'));
  float wa = strtof(wA,0), wb = strtof(wB,0), sign = (op == '-' ? -1 : +1);
  coll_t *A = open_coll (_A, "r+");
  coll_t *B = open_coll (_B, "r+");
  coll_t *P = open_coll (_P, "w+");
  P->rdim = A->rdim;
  P->cdim = A->cdim;
  if (A->rdim != B->rdim || A->cdim != B->cdim) warnx("WARNING: incompatible dimensions %s [%d x %d], %s [%d x %d]", _A, A->rdim, A->cdim, _B, B->rdim, B->cdim);
  uint nA = num_rows (A), nB = num_rows (B), n = MAX(nA,nB), id;
  for (id = 1; id <= n; ++id) {
    ix_t *a = get_vec_ro (A, id);
    ix_t *b = get_vec_ro (B, id);
    ix_t *p = vec_add_vec (wa, a, sign*wb, b);
    put_vec (P, id, p);
    free_vec (p);
  }
  free_coll (P); free_coll (A); free_coll (B);
}

void mtx_eval (char *_TRU, char *_SYS, char *prm) { // thread-unsafe: eval_dump_map
  char *map = strstr(prm,"map"), *roc = strstr(prm,"roc"), *evl = strstr(prm,"evl");
  uint top = getprm(prm,"top=",0);
  //coll_t *SKP = (SKIP && *SKIP) ? open_coll (SKIP, "r+") : NULL;
  coll_t *SYS = open_coll (_SYS, "r+");
  coll_t *TRU = open_coll (_TRU, "r+");
  uint i=0, ni = num_rows(TRU);
  if (map) eval_dump_map (stdout, 0, NULL, NULL); // column headers
  while (++i <= ni) {
    if (!has_vec(TRU,i)) continue; // skip topics with no rels
    ix_t *sys = get_vec (SYS,i);
    ix_t *tru = get_vec (TRU,i);
    if (top) trim_vec (sys, top);
    ixy_t *e = join (sys, tru, -Infinity);
    sort_vec (e, cmp_ixy_X);
    if (evl) eval_dump_evl (stdout, i, e);
    if (roc) eval_dump_roc (stdout, i, e, 1);
    if (map) eval_dump_map (stdout, i, e, prm);
    
    free_vec (e); free_vec(sys); free_vec(tru);
  }
  if (map) eval_dump_map (stdout, 0, NULL, NULL); // column averages

  free_coll(SYS); free_coll(TRU);
}

void mtx_print_f1 (char *SYS, char *TRU, char *prm) {
  int top = getprm (prm,"top=",0);
  float thr = getprm (prm,"thresh=",0);
  float b = getprm (prm,"b=",1), b2 = b*b; // odds of importance of Recall over Precision
  char *B = getprms (prm,"b=","1",',');
  coll_t *S = open_coll (SYS, "r+");
  coll_t *T = open_coll (TRU, "r+");
  uint i, ni = num_rows(T), Q = 0;
  double MP = 0, MR = 0, MF = 0, MAP = 0, MOF = 0, Mre = 0, Mrr = 0;
  //double GP = 0, GR = 0, GF1 = 0, GAP = 0;
  printf ("%6s %4s %4s %6s %6s F%-5.5s %-6s %-6s %-6s\n", "Qry", "Rel", "Rret", "Recall", "Precis", B, "AveP", "maxF1", "thresh");
  for (i = 1; i <= ni; ++i) {
    if (!has_vec(T,i)) continue; // skip topics with no rels
    if (!has_vec(S,i)) {} // do nothing: sys = {}, infinite ranks 
    ix_t *sys = get_vec (S,i), *tru = get_vec (T,i);
    if (top) trim_vec (sys, top);
    if (thr) vec_x_num (sys, 'T', thr);
    double AP = AveP (sys, tru);
    xy_t OF = maxF1 (sys, tru, b);
    vec_x_num (sys, '=', 1);
    vec_x_num (tru, '=', 1);
    double relret = dot(tru,sys), rel = len(tru), ret = len(sys);
    double P = ret ? relret/ret : 0;
    double R = rel ? relret/rel : 0;
    double F = (P+R) ? ((1+b2) * P * R / (b2*P + R)) : 0; 
    printf ("%6d %4.0f %4.0f %.4f %.4f %.4f %.4f %.4f %.4f\n", i, rel, relret, R, P, F, AP, OF.y, OF.x);
    MP  += P;   //GP += log(P);
    MR  += R;   //GR += log(R);
    MF  += F;   //GF1 += log(F1);
    MAP += AP;  //GAP += log(AP);
    MOF += OF.y;
    Mre += rel;
    Mrr += relret;
    ++Q;
    free_vec (sys); free_vec (tru);
  }
  Q = Q ? Q : 1;
  printf ("%6s %4.0f %4.0f %.4f %.4f %.4f %.4f %.4f\n", "avg", Mre/Q, Mrr/Q,     MR/Q,      MP/Q,      MF/Q,      MAP/Q, MOF/Q);
  //printf ("%4s %4.0f %4.0f %.4f %.4f %.4f %.4f\n", "geo",    0.,    0., exp(GR/Q), exp(GP/Q), exp(GF1/Q), exp(GAP/Q));
  free_coll(S); free_coll(T);
}

void mtx_print_evl (char *SYS, char *TRU) {
  coll_t *S = open_coll (SYS, "r+");
  coll_t *T = open_coll (TRU, "r+");
  uint i, nT = num_rows(T);
  for (i = 1; i <= nT; ++i) {
    if (!has_vec(T,i)) continue;
    if (!has_vec(S,i)) {} // fine, sys = {}, infinite ranks 
    ix_t *tru = get_vec (T,i), *sys = get_vec (S,i);
    ixy_t *evl = join (sys, tru, -Infinity);
    sort_vec (evl, cmp_ixy_X);
    eval_dump_evl (stdout, i, evl);
    free_vec (evl); free_vec (sys); free_vec (tru);
  }
  free_coll(S); free_coll(T);
}

void mtx_print_roc (char *SYS, char *TRU, char *prm) {
  char *micro = strstr (prm,"micro"), *ranks = strstr (prm,"ranks");
  int top = getprm (prm,"top=",0), tics = 1 + getprm (prm,"tics=",100), j;
  float b = getprm (prm,"b=",1), minx = getprm (prm,"minx=",-Infinity);
  coll_t *S = open_coll (SYS, "r+");
  coll_t *T = open_coll (TRU, "r+");
  float min = Infinity, max = -Infinity;
  if (!ranks) m_range(S,top,&min,&max);
  //printf ("range: %f ... %f\n", min, max);
  double *np = new_vec(tics,sizeof(double));
  double *nn = new_vec(tics,sizeof(double));
  long double NP = 0, NN = 0, TP = 0, FP = 0;
  uint i, nT = num_rows(T);
  for (i = 1; i <= nT; ++i) {
    if (!has_vec(T,i)) continue;
    if (!has_vec(S,i)) {} // fine, sys = {}, infinite ranks 
    ix_t *sys = get_vec (S,i), *tru = get_vec (T,i);
    if (ranks) {
      ixy_t *evl = join (sys, tru, minx);
      sort_vec (evl, cmp_ixy_X);
      eval_dump_roc (stdout, i, evl, b);
      free_vec (evl); free_vec (sys); free_vec (tru); continue; 
    }
    if (top) trim_vec (sys, top);
    ixy_t *evl = join (sys, tru, -Infinity), *e; // x = system, y = truth
    float c = micro ? 1 : (1./len(tru));
    for (e = evl; e < evl+len(evl); ++e) {
      if (e->x == -Infinity) e->x = min;
      uint x = (tics - 1) * (e->x - min) / (max-min);
      if (e->y > 0) {np[x]+=c; NP+=c;} // positives with score e->x 
      else          {nn[x]+=c; NN+=c;} // negatives with score e->x 
      //printf ("%f %f -> %d\n", e->y, e->x, x);
    }
    free_vec (evl); free_vec (sys); free_vec (tru);
  }
  free_coll(S); free_coll(T);
  if (ranks) { free_vec (np); free_vec (nn); return; }
  printf ("#%5s %6s %6s %6s %3s %10s\n", 
	  "TPR", "FPR", "Prec", "F1", "pct", "score");
  for (j = tics-1; j >= 0; --j) {
    TP += np[j]; FP += nn[j];
    double x = j * (max-min) / (tics-1) + min, pct = 100 * (TP+FP) / (NP+NN);
    double TPR = TP/NP, FPR = FP/NN, Prec = TP/(TP+FP);
    double F1 = (b*b+1)*TPR*Prec / (b*b*Prec + TPR);
    printf ("%6.4f %6.4f %6.4f %6.4f %3.0f %10.4f %3d\n", 
	    TPR, FPR, Prec, F1, pct, x, j);
  }
  free_vec (np); free_vec (nn); 
}

char *strcdr (char *str, char *brk) {
  char *s = strstr(str,brk);
  if (s) {*s=0; return s + strlen(brk);}
  else          return str + strlen(str);
}

static uint in_range (int i, uint min, uint max) {
  uint j = (i < 0) ? (max+i) : (uint)i;
  if (j < min) j = min;
  if (j > max) j = max;
  return j;
}

static it_t parse_slice (char *slice, uint min, uint max) {
  it_t d = {0,0};
  char *LO = slice, *HI = index(LO,':');
  if (HI) *HI++ = 0;
  if (LO && !sscanf (LO, " %d", &(d.i))) d.i = min;  // [:5]
  if (!HI)                               d.t = d.i;  //  [3]
  if (HI && !sscanf (HI, " %d", &(d.t))) d.t = max;  // [3:]
  d.i = in_range (d.i, min, max);
  d.t = in_range (d.t, min, max);
  return d;
}

static uint parse_dim (char *_D) {
  uint dim = 0;
  if (coll_exists(_D)) { //
    coll_t *D = open_coll (_D, "r");
    dim = nvecs (D); // works for hash or mtx
    free_coll (D);
  } else dim = strtod(_D,0);
  return dim;
}

void mtx_rnd (char *RND, char *prm, char *_R, char *_C) {
  float lo = getprm(prm,"lo=",0), hi = getprm(prm,"hi=",1);
  char *sphere = strstr(prm,"sphere"),   *std = strstr(prm,"std");
  char *simplex = strstr(prm,"simplex"), *exp = strstr(prm,"exp");
  char *ones = strstr(prm,"ones"),       *log = strstr(prm,"log");
  uint top = getprm(prm,"top=",0);
  uint R = parse_dim(_R), C = parse_dim(_C), r;
  fprintf (stderr, "%s = %d x %d %s\n", RND, R, C, (ones?ones:"random"));
  coll_t *rnd = open_coll (RND, "w+"); 
  rnd->rdim = R;
  rnd->cdim = C;
  ix_t *vec;
  for (r = 1; r <= R; ++r) {
    if       (sphere) vec = rand_vec_sphere (C);
    else if (simplex) vec = rand_vec_simplex (C);
    else if     (std) vec = rand_vec_std (C);
    else if     (exp) vec = rand_vec_exp (C);
    else if     (log) vec = rand_vec_log (C);
    else if    (ones) vec = const_vec (C, 1);
    else if     (top) vec = rand_vec_sparse (C, top);
    else              vec = rand_vec_uni (C, lo, hi);
    put_vec (rnd, r, vec);
    free_vec (vec);
    show_progress (r, R, "rows");
  }
  free_coll (rnd);
}

void mtx_eye (char *EYE, char *DIM) {
  coll_t *eye = open_coll (EYE, "w+");
  ix_t *vec = const_vec (1, 1);
  uint i, n = eye->rdim = eye->cdim = parse_dim (DIM);
  for (i = 1; i <= n; ++i)
    put_vec (eye, (vec->i=i), vec);
  free_vec (vec);
  free_coll (eye);
}

void mtx_diag (char *_D, char *_M, char *_i) {
  coll_t *D = open_coll (_D, "w+");
  coll_t *M = open_coll (_M, "r+");
  uint i = atoi (_i+strspn(_i,"[( ")), nr = num_rows(M), nc = num_cols(M);
  ix_t *diag = const_vec (1, 1); // each row of D is a singleton
  if (i) {
    D->rdim = D->cdim = M->cdim;
    fprintf (stderr, "putting row %s [%d] onto a diagonal\n", _M, i);
    ix_t *row = get_vec_ro (M,i), *r = row-1, *rEnd = row+len(row);
    while (++r < rEnd) { *diag = *r; put_vec (D, diag->i, diag); }
  } else {
    D->rdim = M->rdim; 
    D->cdim = M->cdim;
    fprintf (stderr, "extracting diagonal of %s [%d,%d]\n", _M, nr, nc);
    for (i = 1; i <= nr; ++i) { // extract diagonal from each row
      ix_t *row = get_vec_ro (M,i), *r = vec_find (row,i);
      if (r) { *diag = *r; put_vec (D, diag->i, diag); }
    }
  }
  free_vec (diag);
  free_coll (D);
  free_coll (M);
}

void mtx_triul (char *_T, char *_M, char *side) {
  char *U = strstr(side,"triu");
  coll_t *T = open_coll (_T, "w+");
  coll_t *M = open_coll (_M, "r+");
  uint i, nr = num_rows(M), nc = num_cols(M);
  T->rdim = M->rdim; 
  T->cdim = M->cdim;
  fprintf (stderr, "extracting %s triangular part of %s [%d,%d]\n", (U?"upper":"lower"), _M, nr, nc);
  for (i = 1; i <= nr; ++i) { // extract diagonal from each row
    ix_t *R = get_vec (M,i), *r = R-1, *end = R+len(R);
    if (U) while (++r < end) r->x *= r->i > i;
    else   while (++r < end) r->x *= r->i < i;
    chop_vec (R);
    put_vec (T, i, R);
    free_vec (R);
  }
  free_coll (T);
  free_coll (M);
}

void mtx_slice (char *_S, char *prm, char *_M, char *dim) { // [ 2:4, 3:5]
  coll_t *S = open_coll (_S, "w+");
  coll_t *M = open_coll (_M, "r+");
  char *renumber = strstr (prm,"renumber");
  char *rdim = dim + strspn (dim,"[( "), *cdim = strcdr (rdim, ",");
  it_t row = parse_slice (rdim, 1, num_rows(M));
  it_t col = parse_slice (cdim, 1, num_cols(M)); 
  S->rdim = renumber ? (row.t-row.i+1) : M->rdim;
  S->cdim = renumber ? (col.t-col.i+1) : M->cdim;
  fprintf (stderr, "%s = %s [ %d:%d , %d:%d ]\n", _S, _M, row.i, row.t, col.i, col.t);
  uint r;
  for (r = row.i; r <= row.t; ++r) {
    ix_t *vec = get_vec (M, r), *v = vec-1, *end = vec+len(vec);
    while (++v < end) 
      if (v->i < col.i || v->i > col.t) v->i = 0;
      else if (renumber) v->i = v->i - col.i + 1;
    chop_vec (vec);
    put_vec (S, (renumber ? (r-row.i+1) : r), vec);
    free_vec (vec);
  }
  free_coll (S); free_coll (M);
}

void mtx_paste (char *_S, char *prm, char *_M[], uint nM) { // [ 2:4, 3:5]
  char *horz = strstr(prm,"horz"), *vert = strstr(prm,"vert");
  coll_t *S = open_coll (_S, "w+");
  uint i, row_base = 0, col_base = 0;
  for (i = 0; i < nM; ++i) {
    coll_t *M = open_coll (_M[i], "r+");
    uint r, nr = num_rows (M), nc = num_cols(M);
    fprintf (stderr, "%s [%d:,%d:] = %s [1:%d,1:%d]\n",
	     _S, row_base+1, col_base+1, _M[i], nr, nc);
    for (r = 1; r <= nr; ++r) {
      ix_t *old = get_or_new_vec (S, row_base + r, sizeof(ix_t));
      ix_t *add = get_vec (M, r), *s;
      for (s = add; s < add+len(add); ++s) s->i += col_base;
      ix_t *new = vec_x_vec (add, '|', old);
      put_vec (S, row_base + r, new);
      free_vec (old); free_vec (add); free_vec (new);
    }
    if (horz) col_base += nc;
    if (vert) row_base += nr;
    free_coll (M);
  }
  free_coll (S);
}

void mtx_shuffle (char *_S, char *_M) {
  coll_t *S = open_coll (_S, "w+");
  coll_t *M = open_coll (_M, "r+");
  uint r = 1, nr = num_rows (M);
  ix_t *perm = rand_vec (nr);
  sort_vec (perm, cmp_ix_x);
  fprintf (stderr, "Shuffling %d rows of %s: 1->%d, 2->%d ...\n", nr, _M, perm[0].i, perm[1].i);
  for (r = 1; r <= nr; ++r) {
    ix_t *row = get_vec_ro (M, r);
    put_vec (S, perm[r-1].i, row);
  }
  free_vec (perm);
  free_coll (S);
  free_coll (M);
}

void mtx_sample (char *_S, char *_M, char *prm) {
  float n = getprm (prm,"n=",0), p = getprm (prm,"p=",0);
  coll_t *S = open_coll (_S, "w+");
  coll_t *M = open_coll (_M, "r+");
  uint r = 1, nr = num_rows (M);
  if (n) fprintf (stderr, "sub-sampling each row of %s to %.0f entries\n", _M, n);
  if (p) fprintf (stderr, "sub-sampling each row of %s with prob. %.2f\n", _M, p);
  assert (p >= 0 && p <= 1);
  for (r = 1; r <= nr; ++r) {
    ix_t *row = shuffle_vec (get_vec_ro (M, r));
    if (n) len(row) = MIN(n,len(row));
    if (p) len(row) = (uint) (p * len(row));
    sort_vec (row, cmp_ix_i);
    put_vec (S, r, row);
    free_vec (row);
  }
  S->cdim = M->cdim;
  free_coll (S); free_coll (M);
}

void mtx_xval (char *_ALL, char *_TRN, char *_TST, char *_VAL, char *prm) {
  coll_t *ALL = open_coll (_ALL, "r+");
  coll_t *TRN = open_coll (_TRN, "w+");  
  coll_t *TST = open_coll (_TST, "w+");
  coll_t *VAL = _VAL ? open_coll (_VAL, "w+") : NULL;
  uint folds = getprm(prm,"/",10);
  uint test = getprm(prm,"fold=",1);
  uint valid= test % folds + 1;
  uint seed = getprm(prm,"seed=",1); 
  uint i, n = num_rows (ALL);
  srandom(seed);
  for (i=1; i<=n; ++i) if (has_vec (ALL,i)) {
      ix_t *vec = get_vec_ro (ALL, i);
      uint this = random() % folds + 1;
      if (VAL && this == valid) put_vec (VAL, i, vec);
      else if   (this == test)  put_vec (TST, i, vec);
      else                      put_vec (TRN, i, vec);
    }
  TRN->rdim = TST->rdim = ALL->rdim; if (VAL) VAL->rdim = ALL->rdim;
  TRN->cdim = TST->cdim = ALL->cdim; if (VAL) VAL->cdim = ALL->cdim;
  free_coll (ALL); free_coll (TRN); free_coll (TST); free_coll (VAL); 
}

void seg_centroid (char *_M, char *prm, char *_X) {
  float t = getprm(prm,"t=",0), p = getprm(prm,"p=",2);
  coll_t *X = open_coll (_X, "r+"), *M = open_coll (_M, "w+");
  uint i, j, n = num_rows (X);
  for (i = 1; i <= n; i = j) {
    ix_t *centroid = get_vec (X,i);
    for (j = i+1; j <= n; ++j) {
      ix_t *vec = get_vec (X,j), *tmp = centroid;
      float d = pnorm(p,centroid,vec), w = 1./(j-i+1);
      if (d > t) { free_vec (vec); break; }
      centroid = vec_add_vec (1-w, centroid, w, vec);
      free_vec (vec); free_vec (tmp);
      show_progress (j, n, "rows segmented");
    }
    put_vec (M, i, centroid); 
    free_vec (centroid);
  }
  free_coll (X); free_coll (M);
}

void thresh_vec (ix_t *vec, float thr) {
  sort_vec (vec, cmp_ix_X);
  uint z = len(vec)-1, a = 0, i = 0, best_i = 0; 
  while (a < z && vec[a].x > thr) ++a; // scroll until a < thresh
  float hi = vec[a].x, lo = vec[z].x, best = 2*PI;
  for (i = a + 1; i < z; ++i) {
    float x = vec[i].x;
    float Lx = hi-x, Li = i-a, L = atan2 (Lx/(hi-lo), Li/(z-a));
    float Rx = x-lo, Ri = z-i, R = atan2 (Rx/(hi-lo), Ri/(z-a));
    if (PI+R-L < best) { best = PI+R-L; best_i = i; }
  }
  len(vec) = best_i;
  sort_vec (vec, cmp_ix_i);
}

void mtx_clump (char *_C, char *_D, char *_T, char *prm) { // C = clump D D.T prm
  coll_t *C = open_coll (_C, "w+");
  coll_t *D = open_coll (_D, "r+");
  coll_t *T = open_coll (_T, "r+");
  uint top = getprm (prm,"top=",0);
  float thresh = getprm (prm,"thresh=",0.1);
  char *hard = strstr (prm,"hard");
  char *recenter = strstr (prm,"recenter");
  uint d, nd = num_rows(D), nc = 0;
  float *skip = new_vec (nd+1, sizeof(float));
  for (d = 1; d <= nd; ++d) {
    show_progress (d, nd, "docs");
    if (skip[d]) continue;
    ix_t *ctr = get_vec (D, d), *c;  trim_vec (ctr, top);
    ix_t *nns = cols_x_vec (T, ctr); vec_x_num (nns, 'T', thresh);
    if (hard) vec_x_full (nns, '!', skip);
    if (recenter) {
      vec_x_num (nns, '=', 1./len(nns));
      free_vec (ctr); ctr = cols_x_vec (D, nns); trim_vec (ctr, top);
      free_vec (nns); nns = cols_x_vec (T, ctr); vec_x_num (nns, 'T', thresh);
      if (hard) vec_x_full (nns, '!', skip);
    }
    put_vec (C, ++nc, nns);
    for (c = nns; c < nns+len(nns); ++c) skip[c->i] = 1;
    assert (skip[d]);
    free_vec (ctr); free_vec (nns); 
  }
  ix_t *single = const_vec(1,1);
  for (d=1; d<=nd; ++d) if (!skip[d]) { single->i = d; put_vec (C, ++nc, single); }
  free_vec (skip); free_vec (single); free_coll (C); free_coll (D); free_coll (T);
  fprintf (stderr, "done: %d docs -> %d clusters, thresh: %.2f\n", nd, nc, thresh);
}

// maximum spanning forest of adjacency matrix A (must be symmetric!)
jix_t *max_span_tree (coll_t *A) { 
  uint nr = num_rows(A), nc = num_cols(A), n = 0; assert (nr == nc);
  fprintf (stderr, "Maximum Spanning Tree (MST) for graph %s [ %d x %d ]\n", A->path, nr, nc);
  jix_t *M = new_vec (nr,sizeof(jix_t)); // j:parent, i:intree, x:maxsim
  while (++n <= nr) {
    jix_t *m, *best = NULL;
    for (m = M; m < M+len(M); ++m) // find node m not in the tree which has
      if (!m->i && (!best || m->x > best->x)) best = m; // max sim to the tree
    assert (best); // some node must not be in the tree (we add 1 per iteration)
    best->i = best-M+1; // add best node to the forest
    if (!best->j) { // best has no parent 
      best->j = best->i; // becomes its own parent (root)
      best->x = 1; // self-similarity of 1
    }
    ix_t *S = get_vec (A,best->i), *s; // all children of best
    for (s = S; s < S+len(S); ++s) { 
      m = M + s->i - 1; // child of best
      if (!m->i && s->x > m->x) { // not in tree & more similar to best
	m->x = s->x;
	m->j = best->i;
      }
    }
    free_vec (S);
    show_progress (n, nr, "nodes added");
  }
  fprintf (stderr, "%d nodes done.\n", n);
  return M;
}

void mtx_mst (char *_M, char *_A) {
  coll_t *A = open_coll (_A, "r+");
  coll_t *M = open_coll (_M, "w+");
  jix_t *mst = max_span_tree (A);
  sort_vec (mst, cmp_jix);
  append_jix (M, mst);
  free_vec (mst);
  free_coll (A);
  free_coll (M);
}

/*
jix_t *connected_components (jix_t *mst) {
  jix_t **path = new_vec (0, sizeof(jix_t*)), **p;
  jix_t *CC = copy_vec (mst), *c, *r;
  for (c = CC; c < CC+len(CC); ++c) { // for each child in forest
    for (r = c; r->j != r->i; r = CC + r->j - 1) { // parent [parent [parent ...
      path = append_vec (path, &r); // collect nodes along path to root
      assert (r >= CC && r < CC+len(CC)); 
    }
    for (p = path; p < path+len(path); ++p) (*p)->j = r->j; // re-assign to root
    path = resize_vec (path,0);
  }
  free_vec (path);
  sort_vec (CC, cmp_jix);
  uint nCC = 0, prev = 0;
  for (c = CC; c < CC+len(CC); ++c) { // for each child in forest
    if (c->j != prev) { prev = c->j; ++nCC; }
    c->j = nCC;
    c->x = 1;
  }
  return CC;
}

*/

ix_t *reachable_from (ix_t *ids, coll_t *G) { // nodes in G reachable from ids
  assert (num_rows(G) == num_cols(G));
  uint N = num_cols(G); 
  ix_t *R = new_vec (N, sizeof(ix_t)), *r;
  for (r = ids; r < ids + len(ids); ++r) R[r->i-1].x = 1; // reachable
  while (1) {
    for (r = R; r < R+N; ++r) if (r->x && !r->i) break; // reachable, not done
    if (r >= R+N) break; // all reachable nodes already done => stop
    r->i = r-R+1; // r is done
    //fprintf (stderr, "%d ->", r->i);
    ix_t *C = get_vec (G, r->i), *c; // all nodes directly connected to r
    for (c = C; c < C+len(C); ++c) R[c->i-1].x = 1; // mark as reachable
    //for (c = C; c < C+len(C); ++c) fprintf (stderr, " %d:%f", c->i, c->x);
    //fprintf (stderr, "\n");
  }
  chop_vec(R);
  return R;
}

void mtx_reachable (char *_R, char *_S, char *_G) {
  coll_t *R = open_coll (_R, "w+"); // reachable
  coll_t *S = open_coll (_S, "r+"); // starting
  coll_t *G = open_coll (_G, "r+"); // affinity
  if (G->rdim != G->cdim) warnx("WARNING: affinity matrix should be square: %s [%d x %d]", _G, G->rdim, G->cdim);
  if (S->cdim != G->rdim) warnx("WARNING: incompatible dimensions %s [%d x %d], %s [%d x %d]", _S, S->rdim, S->cdim, _G, G->rdim, G->cdim);
  R->rdim = S->rdim;
  R->cdim = G->rdim;
  uint n = num_rows(S), i;
  for (i = 1; i <= n; ++i) {
    ix_t *start = get_vec (S,i);
    ix_t *reach = reachable_from (start, G);
    put_vec (R,i,reach);
    free_vec (start); free_vec (reach);
  }
  free_coll (G);  free_coll (S);  free_coll (R);
}

/*
jix_t *connected_components (coll_t *A) { 
  uint nr = num_rows(A), nc = num_cols(A), n = 0, nCC = 0; assert (nr == nc);
  fprintf (stderr, "Connected components for graph %s [ %d x %d ]\n", A->path, nr, nc);
  jix_t *CC = new_vec (nr,sizeof(jix_t)), *c; // j:CC, i:id, x:1
  for (c = CC; c < CC+nr; ++c) c->i = c-CC+1;
  while (1) {
    for (c = CC; c->j; ++c); // find first 
    
    jix_t *m, *best = NULL;
    for (m = M; m < M+len(M); ++m) // find node m not in the tree which has
      if (!m->i && (!best || m->x > best->x)) best = m; // max sim to the tree
    assert (best); // some node must not be in the tree (we add 1 per iteration)
    best->i = best-M+1; // add best node to the forest
    if (!best->j) { // best has no parent 
      best->j = best->i; // becomes its own parent (root)
      best->x = 1; // self-similarity of 1
    }
    ix_t *S = get_vec (A,best->i), *s; // all children of best
    for (s = S; s < S+len(S); ++s) { 
      m = M + s->i - 1; // child of best
      if (!m->i && s->x > m->x) { // not in tree & more similar to best
	m->x = s->x;
	m->j = best->i;
      }
    }
    free_vec (S);
    show_progress (n, nr, "nodes added");
  }
  fprintf (stderr, "%d nodes done.\n", n);
  return M;
}
*/


/*
void mtx_connected (char *_C, char *_A, char *prm) { 
  coll_t *A = open_coll (_A, "r+");
  float thresh = getprm (prm,"thresh=",0);
  float top = getprm (prm,"top=",0);
  uint i, n = num_rows(A); assert (n == num_cols(A));
  uint *P = new_vec (n+1, sizeof(uint));
  for (i = 1; i <= n; ++i) {
    ix_t *adj = get_vec (A,i);
    if (thresh) vec_x_num (adj, '>', thresh);
    else if (top) trim_vec (adj, top);
    if (len(adj)) P[i] = adj[0].i;
    free_vec (adj);
  }
  for (i = 1; i <= n; ++i) P[i] = P[P[P[P[P[i]]]]];
  free_coll (A);
}
*/

/*
void gac (char *SIM, char *prm) {
  coll_t *S = open_coll (SIM, "r+");
  free_coll(S);
}
*/

void mtx_print_letor_rcv (char *_RCV, char *_QRYS, char *_DOCS, char *prm) {
  float p = getprm (prm, "p=", 1), x; //eps = 0.0001
  FILE *RCV = safe_fopen (_RCV,"r");
  coll_t *QRYS = open_coll (_QRYS, "r+");
  coll_t *DOCS = open_coll (_DOCS, "r+");
  uint dim = num_cols (QRYS), q, d;
  char line[1000];
  while (fgets(line,999,RCV)) {
    if (3 != sscanf (line, "%d %d %f\n", &q, &d, &x)) continue;
    ix_t *Q = has_vec (QRYS,q) ? get_vec (QRYS,q) : rand_vec_std (dim);
    ix_t *D = has_vec (DOCS,d) ? get_vec (DOCS,d) : rand_vec_std (dim);
    ix_t *L = vec_x_vec (Q,'-',D), *l;
    printf ("%+.0f qid:%d", x, q);
    for (l = L; l < L+len(L); ++l) printf (" %d:%.6f", l->i, powa(l->x,p));
    printf (" # doc:%d\n", d);
    free_vec (Q); free_vec (D);
  }
  fclose (RCV); free_coll (QRYS); free_coll (DOCS); 
}

void mtx_print_letor (char *_RELS, char *_QRYS, char *_DOCS, char *prm) {
  if (strstr (prm,"rcv")) return mtx_print_letor_rcv (_RELS, _QRYS, _DOCS, prm); 
  float p = getprm (prm, "p=", 1); //, eps = 0.0001;
  coll_t *QRYS = open_coll (_QRYS, "r+");
  coll_t *DOCS = open_coll (_DOCS, "r+");
  coll_t *RELS = open_coll (_RELS, "r+");
  uint q=0, n = num_rows(RELS), dim = num_cols (QRYS);
  while (++q <= n) {
    if (!has_vec (RELS,q)) continue;
    ix_t *Q = has_vec (QRYS,q) ? get_vec (QRYS,q) : rand_vec_std (dim);
    ix_t *R = get_vec (RELS,q), *r;
    for (r = R; r < R+len(R); ++r) {
      ix_t *D = has_vec (DOCS,r->i) ? get_vec (DOCS,r->i) : rand_vec_std (dim);
      ix_t *L = vec_x_vec (Q,'-',D), *l;
      printf ("%+.0f qid:%d", r->x, q);
      for (l = L; l < L+len(L); ++l) printf (" %d:%.6f", l->i, powa(l->x,p));
      printf (" # doc:%d\n", r->i);
      free_vec (D); free_vec (L);
    }
    free_vec (Q); free_vec (R);
  }
  free_coll (RELS); free_coll (QRYS); free_coll (DOCS); 
}

double mtx_letor_eval_x (coll_t *RELS, coll_t *QRYS, coll_t *DOCS, ix_t *W, char *prm) {
  char *R10 = strstr(prm,"R@10"), *MRR = strstr(prm,"MRR");
  char *dump = strstr(prm,"dump");
  char *cos = strstr(prm,"cosi"), *jac = strstr(prm,"jacc");
  char *chi = strstr(prm,"chi2"), *hel = strstr(prm,"hell");
  float p = getprm (prm,"p=",1); //, eps = 0.0001;
  double eval = 0;
  uint q, n = num_rows(RELS), nq = 0, dim = num_cols(QRYS);
  for (q = 1; q <= n; ++q) {         // for each query
    if (!has_vec (RELS,q)) continue;
    ix_t *Q = has_vec (QRYS,q) ? get_vec (QRYS,q) : rand_vec_std (dim);
    ix_t *R = get_vec (RELS,q), *r;
    for (r = R; r < R+len(R); ++r) { // for each doc listed for query
      ix_t *D = has_vec (DOCS, r->i) ? get_vec (DOCS, r->i) : rand_vec_std (dim);
      uint d = r->i;
      r->i = (r->x > 0);
      r->x = (cos ? cosine (Q, D) :
	      jac ? jaccard (Q, D) :
	      chi ? chi2 (Q, D) :
	      hel ? hellinger (Q, D) :
	      W   ? -wpnorm (p, Q, D, W) :
	      -pnorm (p, Q, D) );
      if (dump) printf ("%d %d %.8f\n", q, d, r->x);
      free_vec (D);
    }
    ++nq;
    if (R10 || MRR) {
      sort_vec (R, cmp_ix_X); // scores in decreasing order
      for (r = R; r < R+len(R); ++r) if (r->i) break; // find 1st +
      if      (R10) eval += (r < R+10); // recall at rank 10
      else if (MRR) eval += 1./(r-R+1); // mean reciprocal rank
    } else          eval += (max(R)->i == 1); // top-1 accuracy
    
    //printf ("%d %d ", q, (q == R->i)); print_vec_svm(R,0,0,0);
    free_vec (Q); free_vec (R); 
  }
  printf ("accuracy: %.4f = %.4f / %d\n", (eval/nq), eval, nq);
  return eval/nq;
}

void mtx_letor_SA (char *_RELS, char *_QRYS, char *_DOCS, char *prm) {
  float best = -9999999;
  uint seed = getprm (prm,"seed=",1); srandom(seed);
  uint iter = getprm (prm,"iter=",1000), i;
  coll_t *QRYS = open_coll (_QRYS, "r+");
  coll_t *DOCS = open_coll (_DOCS, "r+");
  coll_t *RELS = open_coll (_RELS, "r+");
  uint dims = num_cols (DOCS);
  for (i=0; i<iter; ++i) {
    ix_t *W = i ? rand_vec_sphere (dims) : const_vec (dims, 1.0);
    float eval = mtx_letor_eval_x (RELS, QRYS, DOCS, W, prm);
    if (eval > best) {
      printf ("%6d %.4f ", i, (best=eval));
      print_vec_csv (W, dims, NULL);
    }
    free_vec (W);
  }
  free_coll (RELS); free_coll (QRYS); free_coll (DOCS); 
}

void mtx_letor_eval (char *_RELS, char *_QRYS, char *_DOCS, char *_WTS, char *prm) {
  coll_t *QRYS = open_coll (_QRYS, "r+");
  coll_t *DOCS = open_coll (_DOCS, "r+");
  coll_t *RELS = open_coll (_RELS, "r+");
  coll_t *WTS  = _WTS ? open_coll (_WTS, "r+") : NULL;
  ix_t *W = WTS ? get_vec (WTS,1) : NULL;
  mtx_letor_eval_x (RELS, QRYS, DOCS, W, prm);
  free_coll (RELS); free_coll (QRYS); free_coll (DOCS); free_coll (WTS); free_vec (W);
}

void mtx_polyex (char *_POLY, char *_ORIG, char *prm) {
  (void) prm;
  coll_t *ORIG = open_coll (_ORIG, "r+");
  coll_t *POLY = open_coll (_POLY, "w+");
  uint id, nr = num_rows (ORIG), nc = num_cols (ORIG);
  for (id=1;id<=nr;++id) {
    ix_t *O = get_vec (ORIG,id), *a, *b, *end = O+len(O);
    ix_t *P = copy_vec (O), new = {0,0};
    for (a = O; a < end; ++a) 
      for (b = O; b <= a; ++b) {
	new.i = nc + a->i * (a->i - 1) / 2 + b->i;
	new.x = a->x * b->x;
	P = append_vec (P, &new);
      }
    put_vec (POLY, id, P);
    free_vec (O); free_vec (P);
  }
  free_coll (ORIG); free_coll (POLY);
}

void mtx_impute (char *_A, char *_B) {
  coll_t *A = open_coll (_A, "w+");
  coll_t *B = open_coll (_B, "r+");
  float  *M = cols_avg (B,1);
  ix_t   *m = full2vec (M);
  //print_vec_svm (m,0,0,0);
  uint i, nr = num_rows(B);
  for (i = 1; i <= nr; ++i) {
    ix_t *b = get_vec (B,i);
    ix_t *a = vec_x_vec (b, '|', m); //, *v;
    //for (v=a; v < a+len(a); ++v) v->x *= 0.99 + 0.02*rnd(); // jitter
    put_vec (A,i,a);
    free_vec (a); free_vec (b);
  }
  free_vec (M); free_vec (m);
  free_coll(A); free_coll(B);
}

jix_t *mtx2jix (coll_t *M) {
  jix_t *result = new_vec (0, sizeof(jix_t));
  uint j = 0, n = num_rows (M);
  while (++j <= n) {
    ix_t *V = get_vec_ro (M,j), *end = V+len(V), *v;
    for (v = V; v < end; ++v) {
      jix_t new = {j, v->i, v->x};
      result = append_vec (result, &new);
    }
  }
  return result;
}

void maxent_train (jix_t *P, coll_t *X, coll_t *W, char *prm) ;
void mtx_maxent (char *_W, char *_Y, char *_X, char *prm) {
  coll_t *X = open_coll (_X, "r+");
  coll_t *Y = open_coll (_Y, "r+");
  coll_t *W = open_coll (_W, "w+");
  jix_t *P = mtx2jix (Y);
  maxent_train (P, X, W, prm);
  free_coll (X); free_coll (Y); free_coll (W); free_vec (P);
}

void maxent2 (coll_t *W, coll_t *Y, coll_t *X, char *prm) ;
void maxent_bin (coll_t *W, coll_t *Y, coll_t *X, char *prm) ;
void mtx_maxent2 (char *_W, char *_Y, char *_X, char *prm) {
  coll_t *W  = open_coll (_W, "w+");
  coll_t *Y  = open_coll (_Y, "r+");
  coll_t *X  = open_coll (_X, "r+");
  if (strstr(prm,"binary")) maxent_bin (W, Y, X, prm);
  else                      maxent2 (W, Y, X, prm);
  free_coll (X); free_coll (Y); free_coll (W); 
}

void passive_aggressive_bin (coll_t *_W, coll_t *_Y, coll_t *_X, char *prm) ;
void passive_aggressive_1vR (coll_t *_W, coll_t *_Y, coll_t *_X, char *prm) ;
void mtx_PA (char *_W, char *_Y, char *_X, char *prm) {
  coll_t *W  = open_coll (_W, "w+");
  coll_t *Y  = open_coll (_Y, "r+");
  coll_t *X  = open_coll (_X, "r+");
  if (strstr(prm,"binary")) passive_aggressive_bin (W, Y, X, prm);
  else                      passive_aggressive_1vR (W, Y, X, prm);
  free_coll (X); free_coll (Y); free_coll (W); 
}

void hmm_viterbi () {
  
}

void hmm_forward (char *_A, char *_S, char *_O) {
  coll_t *A = open_coll (_A, "w+");
  coll_t *S = open_coll (_S, "r+"); 
  coll_t *O = open_coll (_O, "r+"); 
  uint t, N = num_rows(A);
  ix_t *a = get_vec (A, 1), *o, *tmp1, *tmp2;
  for (t = 2; t <= N; ++t) {
    o = get_vec (O, t);
    a = cols_x_vec (S, tmp1=a);
    a = vec_x_vec (o, '*', tmp2=a);
    put_vec (A, t, a);
    free_vec (o); free_vec (tmp1); free_vec (tmp2); 
  }
  free_vec (a);
  free_coll (A); free_coll (S); free_coll (O);
}

void hmm_backward (char *_B, char *_S, char *_O) {
  coll_t *B = open_coll (_B, "w+");
  coll_t *S = open_coll (_S, "r+"); 
  coll_t *O = open_coll (_O, "r+"); 
  uint t, N = num_rows(B);
  ix_t *b = get_vec (B, N), *o, *tmp1, *tmp2;
  for (t = N-1; t > 0; --t) {
    o = get_vec (O, t+1);
    b = vec_x_vec (o, '*', tmp1=b);
    b = cols_x_vec (S, tmp2=b);
    put_vec (B, t, b);
    free_vec (o); free_vec (tmp1); free_vec (tmp2); 
  }
  free_vec (b);
  free_coll (B); free_coll (S); free_coll (O);
}

// D[d] = SUM_ij P[i,j] |X[i,d] - Y[j,d]|^p
void mtx_dcrm (char *_D, char *_P, char *_X, char *_Y, char *prm) {
  coll_t *X = open_coll (_X, "r+"), *Y = open_coll (_Y, "r+");
  coll_t *P = open_coll (_P, "r+"), *D = open_coll (_D, "w+");
  uint nX = num_rows(X), nY = num_rows(Y), nD = num_cols(X), i,j,d;
  float *Df = new_vec (nD+1, sizeof(float)), p = getprm(prm,"p=",2);
  for (i = 1; i <= nX; ++i) {
    ix_t *xi = get_vec_ro (X,i); float *Xi = vec2full (xi, nD, 0); 
    ix_t *pi = get_vec_ro (P,i); float *Pi = vec2full (pi, nD, 0);
    for (j = 1; j <= nY; ++j) {
      ix_t *yj = get_vec_ro (Y,j); float *Yj = vec2full (yj, nD, 0);
      for (d=1; d<=nD; ++d) Df[d] += Pi[j] * powa ((Xi[d]-Yj[d]), p);
      free_vec (Yj);
    }
    free_vec (Xi); free_vec (Pi); 
  }
  ix_t *Dv = full2vec_keepzero (Df);
  put_vec (D, 1, Dv); free_vec (Df); free_vec (Dv);
  free_coll (X); free_coll (Y); free_coll (P); free_coll (D);
}

// S[j,:] = SUM_w topk (P[j,:] .* A[w,:])
void mtx_semg (char *_S, char *_P, char *_A, char *prm) { // thread-unsafe: chk_SCORE
  uint k = getprm (prm,"k=",4);
  coll_t *P = open_coll (_P, "r+"); uint nj = num_rows (P), j;
  coll_t *A = open_coll (_A, "r+"); uint nw = num_rows (A), w;
  coll_t *S = open_coll (_S, "w+"); uint ni = num_cols (P);
  float *SS = chk_SCORE (ni);
  for (j = 1; j <= nj; ++j) {
    ix_t *pj = get_vec_ro (P,j);
    float *Pj = vec2full (pj, ni, 0);
    for (w = 1; w <= nw; ++w) {
      ix_t *Aw = get_vec (A,w), *a;
      vec_x_full (Aw, '*', Pj);
      trim_vec (Aw, k);
      for (a = Aw; a < Aw+len(Aw); ++a) SS[a->i] += a->x;
      free_vec (Aw);
    }
    ix_t *Sj = full2vec (SS);
    put_vec (S, j, Sj);
    free_vec (Sj); free_vec (Pj); 
  }
  free_coll (P); free_coll (A); free_coll (S);
}

/*
mtx_t *open_mtx (char *path, char *access) {
  char r = access[0], c = access[1], h = access[2];
  mtx_t *m = safe_malloc (sizeof(mtx_t));
  if (*access != 'r') mkdir (path, S_IRWXU | S_IRWXG | S_IRWXO);
  c->path = path = strdup(path);
  if (r!='-') c->rows = open_coll (cat(path,"/rows"), access);
  if (c!='-') c->cols = open_coll (cat(path,"/cols"), access);
  if (h!='-') c->rids = open_hash (cat(path,"/rids"), access);
  if (h!='-') c->cids = open_hash (cat(path,"/cids"), access);
}
*/

char *usage = 
  "mtx options             - matrix operations, optional [parameters] are in brackets\n"
  //  " -m 256                 - set mmap window to 256MB (set to ~25% of available RAM)\n"
  " -r 1                   - re-seed the random number generator (1:default, 0:systime)\n"
  " load:fmt M [R] [C] [p] - read matrix M from stdin. Formats: rcv,csv,svm,txt,xml\n"
  "                          hashes R,C used to map string ids -> row/column numbers\n"
  "                          p=xxx access permissions for M,R,C (default:waa)\n"
  "                              w: read/write, delete existing content\n"
  "                              a: read/write, keep existing content\n"
  "                              r: read-only, no new entries, error if doesn't exist\n"
  "                          stem=K,L ... Krovetz,Lowercase stemming (txt,xml)\n"
  "                          gram=4:5 ... character 4- and 5-grams instead of tokens\n"
  "                          char=1   ... character size (in bytes) for n-grams\n"
  "                          ow=5,uw=5 ... ordered/unordered pairs in a 5-word window\n"
  "                          positions ... store word positions instead of frequencies\n"
  "                          join,skip,replace ... documents with duplicate ids\n"
  " print:fmt M [R] [C]    - print matrix M using specified format: rcv,csv,svm,txt,json,ids\n"
  "                          R,C       ... used to map row/column numbers -> string ids\n"
  "                          top=9     ... 9 biggest values per row in descending order\n"
  "                          rid=ABC   ... only row with id=ABC (rno=N for row number)\n"
  "                          empty     ... include empty rows (for csv,svm,txt)\n"
  "                          fmt=' %f' ... csv number format (must be last parameter)\n"
  " print:f1[prm] Sys Tru  - evaluation: recall, precision, F1, AP\n"
  "                          prm: top=K,b=1,thresh=X\n"
  " print:evl Sys Tru      - dump evaluation info for trec_eval\n"
  " print:roc Sys Tru prm  - ROC curve: False-Alarm/Recall/Precision/F1 vs threshold\n"
  "                          prm: tics=K ... number of threshold values (100)\n"
  "                               macro  ... large & small classes have same weight\n"
  "                               micro  ... large classes more important than small\n"
  "                               ranks  ... dump raw ranked lists per class\n"
  "                               top=K  ... truncate to top K scores per class\n"
  " transpose M            - transpose matrix M into M.T (eg docs -> inverted lists)\n"
  " merge A R C += B S D   - merge B[SxD] into A[RxC], re-mapping the row/column ids\n"
  "                          merge[:join,skip,replace] ... rows with clashing ids\n"
  "                          merge,p=aaa ... access mode for A,R,C: read,append,write\n"
  " B = f A [S]            - apply function f to matrix A, store results in B\n"
  "                          S: matrix for computing statistics (default: use A)\n"
  "                          f:   std - make each column zero mean, unit variance\n"
  "                               idf - multiply weight by idf of the feature\n"
  "                               inq - InQuery tf.idf (BM25)\n"
  "                               ntf - BM25-normalized TF (no idf), set k=2,b=0.75\n"
  "                              bm25 - Okapi BM25, set k=2,b=0.75\n"
  "                               rnd - random weights ~ Uniform [0,1] (see -r)\n"
  "                            lm:b=b - language model, Jelink-Mercer smoothing\n"
  "                            lm:k=k - language model, Dirichlet prior k\n"
  "                           clarity - P(w|D) log P(w|D)/P(w) [SIGIR'02]\n"
  "                             rbf=b - radial basis function: exp(-b*x^2)\n"
  "                             sig=b - logistic sigmoid: 1/(1+exp(-b*x)\n"
  "                             erf   - error function: erf (x)\n"
  "                           lsh:L=0 - binarize (>0), split into L chunks\n"
  "                           simhash - generate L=32 fingerprints of k=16 bits\n"
  "                                     sampling simhash:Uniform,Normal,Logistic,Bernoulli\n"
  "                               uni - uniform weights over top=k features\n"
  "                             ranks - replace weights with rank\n"
  "                             top=k - keep only k highest cells in each vector\n"
  "                          thresh=k - keep only cells with values > X\n"
  "                         FS:df=a:b - remove columns with frequency outside [a:b]\n"
  "                         outlier=Z - crop values outside Z standard deviations\n"
  "                              chop - remove zero entries\n"
  "                          mtx2full - convert matrix A to collection of float[]\n"
  "                          full2mtx - convert a collection of float[] to matrix\n"
  "                                L1 - normalize so weights add up to one\n"
  "                                L2 - normalize so that Euclidian length = 1\n"
  "                             round - to nearest integer (also: floor/ceiling)\n"
  "                         Laplacian - divide by sqrt (rowsum * colsum)\n"
  "                           range01 - normalize matrix to unit range [0,1]\n"
  "                             row01 - normalize each row to unit range [0,1]\n"
  "                           log/exp - logarithm/exponential of each cell\n"
  "                         acos/atan - arc cosine / arc tangent of each cell\n"
  "                           sgn/abs - sign / absolute value of each cell\n"
  "                           max/min - max or min of each row (B is rows x 1)\n"
  "                     sum/sum2/sum0 - sum of values / squared / non-zero for each row\n"
  "                           logSexp - log-sum-exp value of each row\n"
  "                             log2p - converts log P(q|d) into posterior P(d|q)\n"
  "                           softmax - exp(x) / SUM_row exp(x) for each x in row\n"
  "                           softcol - exp(x) / SUM_col exp(x) for each x in col\n"
  "                            colmax - keep only the largest cell in each column\n"
  "                                     also: {col,row}{min,max,sum[,p=1]}\n"
  " M = max A B            - cell-wise maximum (also min)\n"
  " M = ones R C           - matrix of ones with R rows and C columns\n"
  " M = rnd:[type] R C     - random matrix with R rows and C columns\n"
  "                          R/C can be numbers or hashes\n"
  "                          type: lo=0,hi=1 - uniformly over [0,1] (default)\n"
  "                                std       - Gaussian with zero mean, unit variance\n"
  "                                exp       - exponential distribution\n"
  "                                log       - logistic distribution\n"  
  "                                sphere    - uniformly over unit sphere\n"
  "                                simplex   - uniformly over unit simplex\n"
  "                                top=K     - U[0,1] over K random dimensions\n"
  " I = eye R              - identity matrix with rows R (R can be hash or matrix)\n"
  " I = diag A [i]         - extract diagonal from A (or put row A[i] onto diagonal)\n"
  " T = triu A             - extract part of A above the diagonal (tril = below)\n"
  " B = slice A [c:d,e:f]  - same as Matlab/Python except rows/columns not renumbered\n"
  "                          use slice:renumber if you want B[1,1] = A[c,e]\n"
  "                          c,d,e,f can be positive, negative, or unspecified\n"
  " A = paste B C D ...    - concatenates matrices without renumbering rows/columns\n"
  "                          use paste:horz or paste:vert to cat renumbered slices\n"
  " A = shuffle B          - randomly re-order (permute) the rows of B (see -r)\n"
  " A = sample:[type] B    - down-sample each row to n=N items or with prob. p=P\n"
  " A = subset B H         - read ids from stdin and set A[id] = B[id] using hash H\n"
  " xval:fold=1 A T E [V]  - cross-validation A:all, T:train, E:test, V:validation\n"
  " T = mst A              - max spanning tree of affinity matrix A (symmetric)\n"
  " R = reachable S G      - R[i,:] = nodes in graph G reachable from S[i,:]\n"
  " C = seg:[type] A       - segment a sequence of observations (A)\n"
  "                          type: centr,t=0 - agglomerate while ||centr-row|| < t\n"
  " C = clump:[prm] X X.T  - fast clustering algorithm for rows of X\n"
  "                          prm: thresh=0.1\n"
  " LTR:out,p=1 R Q D      - dump LeToR vectors: Rij |Qi1-Dj1|^p ... |Qin-Djn|^p\n"
  " LTR:SA,p=1 R Q D       - learn LeToR weights via simulated annealing\n"
  " LTR:eval R Q D [W]     - evaluate LeToR, prm: dump,cosi/jacc/chi2/p=0.5,MRR,R@10\n"
  " A = polyex B           - sparse polynomial expansion on columns of B\n"
  " A = impute B           - fill missing values of B with column means\n"
  " W = maxent:[prm] Y X   - multi-class regularised logistic regression\n"
  "                          iter=10,faster=1.1,slower=0.5,rate=0.1,ridge=0.3\n"
  "                          valid=0,vocab=0,top=0,seed,mask\n"
  " W = PA:[prm] Y X       - passive-aggressive algorithm I, prm:iter=10,C=1,binary\n"
  " W = CD:[prm] Y X.T     - coordinate-descent SVM, prm: iter=10,c=0,p=1\n"
  " W = svm:[prm] C K      - learn SVM for classes in C based on kernel matrix K\n"
  "                          classify: Y = K x W.T, where K is testing x training\n"
  " D = dcrm[p=2] P X Y    - CRM gradient: D[d] = SUM_ij P[i,j] |X[i,d] - Y[j,d]|^p\n"
  " S = semg[k=4] P A      - semantic group: S[j,:] = SUM_w topk (P[j,:] .* A[w,:])\n"
  " P = A x B.T [type]     - multiply matrix A by B.T: P[r,c] = A[r,:] * B.T[:,c]\n"
  "                          rows of A must be compatible w. rows of B (cols of B.T)\n"
  "                          type: cosine    - cosine/Ochiai coefficient\n"
  "                                jaccard   - Jaccard/Tanimoto coefficient\n"
  "                                dice      - Dice/Sorensen coefficient\n"
  "                                chi2      - 1-chi^2 distance\n"
  "                                hellinger - 1-Hellinger distance\n"
  "                                euclidean - negative Euclidean distance\n"
  "                                manhattan - negative Manhattan distance\n"
  "                                hamming   - negative Hamming distance\n"
  "                                norm,p=x  - negative Minkowski distance (p-norm)\n"
  "                                            NORM,p=x same, but without p'th root\n"
  "                                lm        - convert to LM: j=0.2, d=100, docs=DOCS\n"
  "                                BAND      - Boolean AND\n"
  "                                slink     - P[i,j] = max_w min {A[i,w], B[j,w]}\n"
  "                                maxx/minx - P[i,j] = max/min_w {A[i,w] * B[w,j]}\n"
  "                                smax/smin - P[i,j] = SUM_w max/min {A[i,w], B[w,j]}\n"
//"                                rbf=B     - convert to RBF kernel: exp(-b*P[i,j]^2)\n"
  "                                top=K     - keep K highest values in each row of P\n"
  "                                thresh=X  - keep only values > X in each row of P\n"
//"                                keepzero  - keep zero values in the matrix P\n"
  " P = A + B              - add matrix A to B: P[r,c] = A[r,c] + B[r,c]\n"
  "                          also supports: +,-,.,/,^,&,|,!,<,>\n"
  "                          rows of A must be compatible with rows of B\n"
  "                          either A or B can be scalar\n"
  " P = x A + y B          - A,B must be compatible matrices; x,y scalars, can do +,-\n"
  " D = A dist:[type] B    - negative distance matrix: D[i,j] = - ||A[i,:] - B[j,:]||\n"
  " D = A # B [type]                                                                 \n"
  "                          type: p=0/1/2/0.x - Hamming/Manhattan/Euclidian/Minkowski\n"
  "                                rbf=b       - distance -> kernel w. bandwidth b\n"
  "                                cos/corr/R2 - cosine, Pearson correlation, R-squared\n"
  "                                top=K       - keeps only K closest components\n"
  "                                              (put this *after* p=P)\n"
  "                                kdot=K      - dot restricted to K largest terms\n"

  "                                chi2        - D[i,j] = chi-squared (A[i,:], B[j,:])\n"
  "                                min         - D[i,j] =     min { A[i,:] & B[j,:] }\n"
  "                                max         - D[i,j] =     max { A[i,:] & B[j,:] }\n"
  "                                logSexp     - D[i,j] = logSexp { A[i,:] & B[j,:] }\n"
  "                                wts=W       - weighted Minkowski distance\n"
//"                                subset      - D specifies which pairs to compare\n"
  " size[:r/c] A           - report the dimensions of A (rows/cols)\n"
  " norm[:p=1] A           - p-norm of A: SUM_r,c A[r,c]^p\n"
  " trace[:avg] A          - sum/average of elements on the diagonal of A\n"
  "\nExamples: http://bit.ly/irtool\n\n"
  ;

#define arg(i) ((i < argc) ? argv[i] : NULL)
#define a(i) ((i < argc) ? argv[i] : "")

int remove_sugar(int n, char *A[]) { // DOCS [docs x words] --> DOCS docs words
  if (strncmp(A[1], "load", 4) && 
      strncmp(A[1], "print", 5) &&
      strncmp((n>3?A[3]:""), "ones", 4) &&
      strncmp((n>3?A[3]:""), "rnd", 3)) return n; // nothing to do
  int i, j;
  for (i = j = 1; i < n; ++i) {
    int z = strlen(A[i]) - 1; // index of last character
    if (z==0 && index(":[]x,",A[i][0])) continue; // skip solitary :[]x,
    //printf ("argv[%d] = '%s'",j,A[i]);
    if (A[i][z] == ']') { A[i][z] = 0; } //printf (", chop trailing ']' -> '%s'", A[i]); }
    if (A[i][z] == ',') { A[i][z] = 0; } //printf (", chop trailing ',' -> '%s'", A[i]); }
    if (A[i][0] == '[') { A[i] += 1;   } //printf (", chop leading ']'  -> '%s'", A[i]); }
    //printf ("\n");
    A[j++] = A[i];
  }
  return j;
}

int main (int argc, char *argv[]) {
  vtime();
  if (argc < 3) { printf ("%s", usage); return 1; }
  if (!strcmp (a(1), "-m")) {
    MAP_SIZE = ((ulong) atoi (a(2))) << 20; argv+=2; argc-=2; 
    fprintf (stderr, "MAP_SIZE is %uMB\n", (uint)(MAP_SIZE>>20)); }
  if (!strcmp (a(1), "-r")) {
    uint seed = atoi (a(2)); if (!seed) seed = (uint)time(0); srandom (seed);
    fprintf (stderr, "RNG SEED is %u\n", seed); argv+=2; argc-=2; }
  argc = remove_sugar (argc, argv);
  if      (!strncmp(a(1), "size", 4))   mtx_size (arg(2), a(1));
  else if (!strncmp(a(1), "norm", 4))   mtx_norm (arg(2), a(1));
  else if (!strncmp(a(1), "trace", 5))  mtx_trace (arg(2), a(1));
  else if (!strncmp(a(1), "load:", 5))  mtx_load (arg(2), arg(3), arg(4), a(1)+5, a(5));
  else if (!strncmp(a(1),"print:f1",8)) mtx_print_f1  (arg(2), arg(3), a(1));
  else if (!strcmp (a(1), "print:evl")) mtx_print_evl (arg(2), arg(3));
  else if (!strcmp (a(1), "print:roc")) mtx_print_roc (arg(2), arg(3), a(4));
  else if (!strncmp(a(1), "print", 5))  mtx_print (arg(1), arg(2), arg(3), arg(4));
  else if (!strncmp(a(1), "LTR:out", 7)) mtx_print_letor (arg(2), arg(3), arg(4), a(1));
  else if (!strncmp(a(1), "LTR:SA", 6)) mtx_letor_SA (arg(2), arg(3), arg(4), a(1));
  else if (!strncmp(a(1), "LTR:eval", 8)) mtx_letor_eval (arg(2), arg(3), arg(4), arg(5), a(1));
  else if (!strncmp(a(1), "xval", 4))   mtx_xval (arg(2), arg(3), arg(4), arg(5), a(1));
  else if (!strncmp(a(1), "trans", 5))  mtx_transpose (arg(1), arg(2), NULL);
  else if (!strncmp(a(1), "merge", 5) 
	   && !strcmp (a(5),"+="))    mtx_merge (arg(2), arg(3), arg(4), arg(6), arg(7), arg(8), a(1));
  else if (!strcmp(a(2),"=") && argc > 3) {
    char tmp[1000]; 
    sprintf (tmp, "%s.%d", arg(1), getpid()); // temporary target
    if      (argc == 4 && atof(a(3)))      mtx_dot (tmp, arg(1), '=', arg(3));
    else if (argc == 4)                    mtx_copy (arg(3), tmp); // cp_dir (arg(3), tmp);
    else if (!strcmp  (a(3), "ones"))      mtx_rnd (tmp, arg(3), arg(4), arg(5));
    else if (!strncmp (a(3), "rnd",3))     mtx_rnd (tmp, arg(3), arg(4), arg(5));
    else if (!strcmp  (a(3), "eye"))       mtx_eye (tmp, arg(4));
    else if (!strcmp  (a(3), "diag"))      mtx_diag (tmp, arg(4), a(5));
    else if (!strcmp  (a(3), "triu"))      mtx_triul (tmp, arg(4), a(3));
    else if (!strcmp  (a(3), "tril"))      mtx_triul (tmp, arg(4), a(3));
    else if (!strcmp  (a(3), "svm"))       svm_train (tmp, arg(4), arg(5));
    else if (!strncmp (a(3), "CD",2))      cd_train_1vR (tmp, arg(4), arg(5), arg(3));
    else if (!strncmp (a(3), "seg",3))     seg_centroid (tmp, arg(3), arg(4));
    else if (!strncmp (a(3), "slice",5))   mtx_slice (tmp, arg(3), arg(4), arg(5));
    else if (!strncmp (a(3), "paste",5))   mtx_paste (tmp, arg(3), argv+4, argc-4);
    else if (!strcmp  (a(3), "shuffle"))   mtx_shuffle (tmp, arg(4));
    else if (!strncmp (a(3), "sample",6))  mtx_sample (tmp, arg(4), a(3));
    else if (!strncmp (a(3), "subset",6))  mtx_subset (tmp, arg(4), arg(5));
    else if (!strncmp (a(3), "polyex",6))  mtx_polyex (tmp, arg(4), a(3));
    else if (!strncmp (a(3), "impute",6))  mtx_impute (tmp, arg(4));
    else if (!strncmp (a(3), "maxent",6))  mtx_maxent2 (tmp, arg(4), arg(5), a(3));
    else if (!strncmp (a(3), "PA:",3))     mtx_PA (tmp, arg(4), arg(5), a(3));
    else if (!strncmp (a(3), "dcrm",4))    mtx_dcrm (tmp, arg(4), arg(5), arg(6), a(3));
    else if (!strncmp (a(3), "semg",4))    mtx_semg (tmp, arg(4), arg(5), a(3));
    else if (!strncmp (a(3), "clump",5))   mtx_clump (tmp, arg(4), arg(5), a(3));
    else if (!strcmp  (a(3), "mst"))       mtx_mst (tmp, arg(4));
    else if (!strcmp  (a(3), "reachable")) mtx_reachable (tmp, arg(4), arg(5));
    else if (!strncmp (a(4), "dist",4))    mtx_distance (tmp, arg(3), arg(5), arg(4));
    else if (!strcmp  (a(4), "#"))         mtx_distance (tmp, arg(3), arg(5), arg(6));
    else if (!strcmp  (a(4), "x"))         mtx_product (tmp, arg(3), arg(5), arg(6));
    else if (!strcmp  (a(5), "+") ||
	     !strcmp  (a(5), "-"))         mtx_add (tmp, arg(3), arg(4), arg(5)[0], arg(6), arg(7));
    else if (!strcmp  (a(3), "min") && argc == 6) mtx_dot (tmp, arg(4), 'm', arg(5));
    else if (!strcmp  (a(3), "max") && argc == 6) mtx_dot (tmp, arg(4), 'M', arg(5));
    else if (!strcmp  (a(3), "transpose")) mtx_transpose (arg(3), arg(4), tmp);
    else if (strlen(a(4))==1 && ispunct(a(4)[0])) mtx_dot (tmp, arg(3), arg(4)[0], arg(5));
    else                                   mtx_weigh (tmp, arg(3), arg(4), arg(5));
    //else if (!strncmp (a(3), "weigh",5)) mtx_weigh (tmp, arg(3), arg(4), arg(5));
    mv_dir (tmp, arg(1));
  }
  /*  else {
  }
  */
    /*    if (!strcmp(*a, "-sim")) {
      coll_t  *r = open_coll (a(1),"r+"), *c = open_coll (a(2),"w+");
      transpose_mtx (r, c);
      free_coll (r); free_coll (c); 
    }
    */
    return 0;
}

/*
void mtx_print_table (char *_M) {
  coll_t *M = open_coll (_M, "r+");
  uint i, j, nr = num_rows (M), nc = num_cols (M);
  for (i = 1; i <= nr; ++i) {
    ix_t *vec = get_vec (M, i);
    float *full = vec2full (vec, nc);
    for (j = 1; j <= nc; ++j) printf (" %3.0f", full[j]);
    printf ("\n");
    free_vec(vec); free_vec(full);
  }
  free_coll (M);
}
*/
