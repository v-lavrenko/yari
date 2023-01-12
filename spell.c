/*
  
  Copyright (c) 1997-2021 Victor Lavrenko (v.lavrenko@gmail.com)
  
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

#include "hash.h"
#include "matrix.h"
#include "textutil.h"
#include "timeutil.h"

void dump_levenstein_cost (uint **C, char *A, char *B) {
  uint nA = strlen(A), nB = strlen(B), a, b;
  printf("  ");
  for (b = 0; b <= nB; ++b) printf("  %c", b?B[b-1]:'.');
  printf("\n"); 
  for (a = 0; a <= nA; ++a) {
    printf (" %c",a?A[a-1]:'.');
    for (b = 0; b <= nB; ++b) printf ("%3d", C[a][b]);
    printf("\n");
  }  
}

void dump_levenstein_path (char **D, char *A, char *B) {
  uint nA = strlen(A), nB = strlen(B), a, b;
  printf("  ");
  for (b = 0; b <= nB; ++b) printf("  %c", b?B[b-1]:'.');
  printf("\n"); 
  for (a = 1; a <= nA; ++a) {
    printf (" %c ",a?A[a-1]:'.');
    for (b = 0; b <= nB; ++b) printf ("  %c", D[a][b]);
    printf("\n");
  }
}

void explain_levenstein (char **D, char *A, char *B, char *buf) {
  int nA = strlen(A), nB = strlen(B), a=nA, b=nB; *buf = '\0';
  while (a>0 && b>0) {
    if      (D[a][b] == '^') {--a;         buf += sprintf(buf,"%d--%c,",a+1,A[a]);}
    else if (D[a][b] == '<') {--b;         buf += sprintf(buf,"%d++%c,",b+1,B[b]);}
    else if (D[a][b] == 'S') {--a; --b;    buf += sprintf(buf,"%d>>%c%c,",a+1,A[a],B[b]);}
    else if (D[a][b] == 'T') { a-=2; b-=2; buf += sprintf(buf,"%dtx%c%c,",a+1,A[a],A[a+1]); }
    else if (D[a][b] == '=') {--a; --b; }
  }
}

uint levenstein_distance (char *A, char *B, char *explain) {
  uint nA = strlen(A), nB = strlen(B), a, b;
  uint **C = (uint **) new_2D (nA+1,nB+1,sizeof(uint));
  char **D = (char **) new_2D (nA+1,nB+1,sizeof(char));
  for (a = 0; a <= nA; ++a) C[a][0] = a;
  for (b = 0; b <= nB; ++b) C[0][b] = b;
  for (a = 1; a <= nA; ++a) {
    for (b = 1; b <= nB; ++b) {
      uint del = C[a-1][b] + 1;
      uint ins = C[a][b-1] + 1;
      uint fit = (A[a-1] == B[b-1]);
      uint sub = C[a-1][b-1] + (fit ? 0 : 1);
      uint txp = (a > 1 && A[a-2] == B[b-1] &&
		  b > 1 && A[a-1] == B[b-2]) ? C[a-2][b-2] + 1 : Infinity;
      uint best = MIN(MIN(ins,del),MIN(sub,txp));
      C[a][b] = best;
      D[a][b] = (ins == best) ? '<' : (del == best) ? '^' : (txp == best) ? 'T' : fit ? '=' : 'S';
    }
  }  
  uint result = C[nA][nB];  
  dump_levenstein_cost (C, A, B);
  dump_levenstein_path (D, A, B);
  if (explain) explain_levenstein (D, A, B, explain);
  free_2D((void **)C);
  free_2D((void **)D);
  return result;
}

uint levenstein_uint (uint *A, uint *B) {
  uint nA = len(A), nB = len(B), a, b;
  uint **C = (uint **) new_2D (nA+1,nB+1,sizeof(uint));
  char **D = (char **) new_2D (nA+1,nB+1,sizeof(char));
  for (a = 0; a <= nA; ++a) C[a][0] = a;
  for (b = 0; b <= nB; ++b) C[0][b] = b;
  for (a = 1; a <= nA; ++a) {
    for (b = 1; b <= nB; ++b) {
      uint del = C[a-1][b] + 1;
      uint ins = C[a][b-1] + 1;
      uint fit = (A[a-1] == B[b-1]);
      uint sub = C[a-1][b-1] + (fit ? 0 : 3); // 3 => ins+del cheaper than sub
      uint best = MIN(MIN(ins,del),sub);
      C[a][b] = best;
      D[a][b] = (ins == best) ? '<' : (del == best) ? '^' : fit ? '=' : 'S';
    }
  }
  uint result = C[nA][nB];  
  free_2D((void **)C);
  free_2D((void **)D);
  return result;
}

ix_t *levenstein_track (char **D, uint *A, uint *B) {
  ix_t *ops = new_vec(0,sizeof(ix_t)), op = {0,0};
  int nA = len(A), nB = len(B), a=nA, b=nB;  
  while (a>0 && b>0) {
    if      (D[a][b] == '=') {--a; --b; op.x = 0; op.i = A[a]; } // keep A[a]
    else if (D[a][b] == '^') {--a;      op.x =-1; op.i = A[a]; } // del A[a]
    else if (D[a][b] == '<') {--b;      op.x =+1; op.i = B[b]; } // ins B[b]
    ops = append_vec (ops, &op);
  }
  reverse_vec(ops);
  return ops; // uint count (ix_t *V, char op, float x) {
}

uint levenstein_n_del (ix_t *track) { return count (track,'<',0); }
uint levenstein_n_ins (ix_t *track) { return count (track,'>',0); }
uint levenstein_edits (ix_t *track) { return count (track,'!',0); }

//////////////////////////////////////////////////////////////////////////////// Norvig spell

// delete/transpose/insert/substitute 'a' into string at X[0..n) at position i
int do_edit (char *X, int n, int i, char op, char a) {
  int ok = 0;
  if (op == '=' && i < n) {ok=1; X[i] = a; } // substitute 
  if (op == '-' && i < n)   {ok=1; while (++i <= n) X[i-1] = X[i]; } // delete 
  if (op == '^' && i < n-1) {ok=1; char tmp=X[i]; X[i]=X[i+1]; X[i+1]=tmp; } // transpose
  if (op == '+' && i < n+1) {ok=1; do X[n+1] = X[n]; while (--n >= i); X[i] = a; } // insert
  return ok;
}

// insert all edits of word into set
int all_edits (char *word, uint nedits, hash_t *edits) {
  int n = strlen(word), pos;
  char *edit = calloc(n+3, 1);
  char *op, *ops = nedits > 2 ? "-^" : "-^=+"; // no ins/sub if 3+ edits
  for (op = ops; *op; ++op) { // no substitution if deleting/transposing
    char *sub, *subs = (*op=='-' || *op=='^') ? "a" : "abcdefghijklmnopqrstuvwxyz";
    for (sub = subs; *sub; ++sub) {
      for (pos = 0; pos <= n; ++pos) {
	memcpy(edit,word,n+1);
	int ok = do_edit (edit,n,pos,*op,*sub);
	if (!ok) continue; // could not do an edit
	if (nedits > 1) all_edits (edit, nedits-1, edits); // more edits to be done
	else if (edits) key2id (edits,edit); // leaf => insert into hash
      }
    }
  }
  free(edit);
  return 0;
}

// map edits to known words, assign scores
ix_t *score_edits (hash_t *edits, hash_t *known, float *score) {
  uint i, n = nkeys(edits);
  assert(len(score) > nkeys(known));
  ix_t *result = new_vec(0,sizeof(ix_t)), new = {0,0};
  for (i = 1; i <= n; ++i) {
    char *edit = id2key (edits, i);
    new.i = has_key (known, edit);
    new.x = new.i ? score[new.i] : 0;
    if (new.i) result = append_vec(result, &new);
  }
  return result;
}

// *results = all edits that match the known hash, with scores
int known_edits (char *word, uint nedits, hash_t *known, float *score, ix_t **result) {
  int n = strlen(word), pos; ix_t new = {0,0};
  char *edit = calloc(n+3, 1);
  char *op, *ops = nedits > 2 ? "-^" : "-^=+"; // no ins/sub if 3+ edits
  for (op = ops; *op; ++op) { // no substitution if deleting/transposing
    char *sub, *subs = (*op=='-' || *op=='^') ? "a" : "abcdefghijklmnopqrstuvwxyz";
    for (sub = subs; *sub; ++sub) {
      for (pos = 0; pos <= n; ++pos) {
	memcpy(edit,word,n+1);
	int ok = do_edit (edit,n,pos,*op,*sub);
	if (!ok) continue; // could not do an edit
	if (nedits > 1) known_edits (edit, nedits-1, known, score, result); // more edits
	else {
	  new.i = has_key (known, edit); // is this edit a known word?
	  new.x = new.i ? score[new.i] : 0; // 
	  if (new.i) *result = append_vec (*result, &new);
	}
      }
    }
  }
  free(edit);
  return 0;
}

// *best = id of edit that has highest score
int best_edit (char *word, uint nedits, hash_t *known, float *score, uint *best) {
  int n = strlen(word), pos, pos1 = 0; //nedits < 2 ? 1 : 2;
  char *edit = calloc(n+3, 1);
  char *op, *ops = nedits > 2 ? "-^" : "-^=+"; // no ins/sub if 3+ edits
  for (op = ops; *op; ++op) { // no substitution if deleting/transposing
    char *sub, *subs = (*op=='-' || *op=='^') ? "a" : "abcdefghijklmnopqrstuvwxyz";
    for (sub = subs; *sub; ++sub) {
      for (pos = pos1; pos <= n; ++pos) {
	memcpy(edit,word,n+1);
	int ok = do_edit (edit,n,pos,*op,*sub);
	if (!ok) continue; // could not do an edit
	if (nedits > 1) best_edit (edit, nedits-1, known, score, best); // more edits
	else {
	  uint id = has_key (known, edit); // is this edit a known word?
	  if (id && score[id] > score[*best]) *best = id;
	}
      }
    }
  }
  free(edit);
  return 0;
}

// aminoacid -> {i:amino, j:acid, x: MIN(F[amino],F[acid])}
void try_runon (char *word, hash_t *known, float *score, uint *_i, uint *_j) {
  float best = -Infinity;
  int n = strlen(word), k;
  for (k = 3; k < n-2; ++k) {
    char *wi = strndup(word,k), *wj = strndup(word+k,n-k);
    uint i = has_key(known,wi), j = has_key(known,wj);
    float x = (i && j) ? MIN(score[i],score[j]) : -Infinity;
    if (x > best) { best=x; *_i=i; *_j=j; }
    free(wi); free(wj);
  }
}

// amino acid -> aminoacid
uint try_fuse (char *w1, char *w2, hash_t *known) {
  int n1 = strlen(w1), n2 = strlen(w2);
  char *w12 = calloc(n1+n2+1,1); strcat(w12,w1); strcat(w12,w2);
  uint i12 = has_key(known,w12);
  free(w12);
  return i12;
}

uint pubmed_spell (char *word, hash_t *H, float *F, char *prm, uint W, uint *id2) {
  char V = prm && strstr(prm,"verbose") ? 1 : 0; // getprm(NULL) is fast
  //L1=4,L2=9,F0=1000,F1=10,F2=10000,F3=10000,F11=10000,x1=100,x10=10000,x11=10000,x20=10000,x21=10000,x30=10000,x31=10000,x32=10000
  uint L1  = getprm(prm,"L1=",5);      // do not correct if len(w) < L1
  uint L2  = getprm(prm,"L2=",9);      // no 2-edits if len(w) < L2
  uint F0  = getprm(prm,"F0=",1000);   // do not correct if F[w] > F0
  uint F1  = getprm(prm,"F1=",10);     // reject 1-edit if F[w1] < F1
  uint x1  = getprm(prm,"x1=",100);    // reject 1-edit if F[w1] < x1 F[w0]
  uint F2  = getprm(prm,"F2=",100);    // reject 2-edit if F[w2] < F2
  uint F3  = getprm(prm,"F3=",100);    // reject 2+1edit if F[w21] < F3
  uint F11 = getprm(prm,"F11=",100);   // reject 1+1edit if F[w11] < F11
  uint x10 = getprm(prm,"x10=",100);   //  10 ~ 100 ~ 1k
  uint x11 = getprm(prm,"x11=",10);    // *1k >> 100 > 10
  uint x20 = getprm(prm,"x20=",100);   //  1k ~ 10 >> 1
  uint x21 = getprm(prm,"x21=",10);    //  100 > 10 >> 1
  uint x30 = getprm(prm,"x30=",1000);  //  1k ~ 10 ~ 100
  uint x31 = getprm(prm,"x31=",100);   //  1k ~ 10 ~ 100
  uint x32 = getprm(prm,"x32=",10);    //  1k ~ 100 ~ 1 ~ 10
  
  uint w0 = has_key (H, word), w1=0, w2=0, wL=0, wR=0, w11=0, w21=0, w3=0;
  uint l0 = strlen (word), id = 0, ok = 0; (void) id2; (void) w11;
  //if (V) printf ("%s\n", word);  
  ok = (F[w0] >= F0);
  if (V) printf ("%d %d\tas-is: %s:%.0f\n", w0==W, ok, word, F[w0]);
  if (ok) { id=id?id:w0; if (!V) return id; }
  
  if ((l0 >= L1) || !w0) { // if word long enough, or zero matches: try 1-edit
    
    best_edit (word, 1, H, F, &w1); // w1 = 1-edit(w0)
    ok = (F[w1] > x1*F[w0] && F[w1] >= F1);
    if (V) printf ("%d %d\t1edit: %s:%.0f -> %s:%.0f\n", w1==W, ok, word, F[w0], id2key(H,w1), F[w1]);
    if (ok) { id=id?id:w1; if (!V) return id; }
    
    if (w1) best_edit (id2key(H,w1), 1, H, F, &w11);
    ok = (F[w11] > x11*F[w1] && F[w11] > x10*F[w0] && F[w11] > F11);
    if (V) printf ("%d %d\t1-1ed: %s:%.0f -> %s:%.0f\n", w11==W, ok, word, F[w0], id2key(H,w11), F[w11]);
    if (ok) { id=id?id:w11; if (!V) return id; }
  }
  
  if ((l0 >= L1 && l0 >= L2 && l0 <= 20) || (!id && !w0 && l0 <= 20)) {
    
    best_edit (word, 2, H, F, &w2); // w2 = 2-edit(w0)
    ok = (F[w2] > x21*F[w1] && F[w2] > x20*F[w0] && F[w2] > F2);
    if (V) printf ("%d %d\t2edit: %s:%.0f -> %s:%.0f\n", w2==W, ok, word, F[w0], id2key(H,w2), F[w2]);
    if (ok) { id=id?id:w2; if (!V) return id; }
    
    if (w2) best_edit (id2key(H,w2), 1, H, F, &w21);
    ok = (F[w21] > x32*F[w2] && F[w21] > x31*F[w1] && F[w21] > x30*F[w0] && F[w21] > F3);
    if (V) printf ("%d %d\t2-1ed: %s:%.0f -> %s:%.0f\n", w21==W, ok, word, F[w0], id2key(H,w21), F[w21]);
    if (ok) { id=id?id:w21; if (!V) return id; }
    
    try_runon (word, H, F, &wL, &wR); // wL,R = split(w0)
    ok = (MIN(F[wL],F[wR]) > 500 || !w2);
    if (V) printf ("- %d\trunon: %s:%.0f -> %s:%.0f", ok, word, F[w0], id2key(H,wL), F[wL]);
    if (V) printf (" + %s:%.0f\n", id2key(H,wR), F[wR]);
    if (ok) {if (!id) {id=wL; if (id2) *id2=wR;} if (!V) return id; }
  }

  (void) w3;
  //  if (!id && !w0) { // no matches
  //    best_edit (word, 3, H, F, &w3); // w3 = 3-edit(w0)
  //    ok = (F[w3] > 0);
  //    if (V) printf ("%d %d\t3edit: %s:%.0f -> %s:%.0f\n", w3==W, ok, word, F[w0], id2key(H,w3), F[w3]);
  //    if (ok) { id=id?id:w3; if (!V) return id; }    
  //  }
  
  return id ? id : w0;
}

uint pickmax_spell (char *word, hash_t *H, float *F, char *_, uint W) {
  char V = !strstr(_,"quiet") && !strstr(_,"silent") ? 1 : 0; // verbose?
  float x1  = getprm(_,"x1=",1), x11 = getprm(_,"x11=",1);
  float x2  = getprm(_,"x2=",1), x21 = getprm(_,"x21=",1);
  
  uint w0 = has_key (H, word), w1=0, w2=0, w11=0, w21=0, id = w0;
  best_edit (word, 1, H, F, &w1);                   // w1  = 1-edit(w0)
  if (w1) best_edit (id2key(H,w1), 1, H, F, &w11);  // w11 = 1-edit(w1)
  best_edit (word, 2, H, F, &w2);                   // w2  = 2-edit(w0)
  if (w2) best_edit (id2key(H,w2), 1, H, F, &w21);  // w21 = 1-edit(w2)
  float f0 = F[w0], f1 = F[w1]/x1, f11 = F[w11]/x11, f2 = F[w2]/x2, f21 = F[w21]/x21, best = f0;
  if (f1 > best)  { best = f1; id = w1; }
  if (f2 > best)  { best = f2; id = w2; }
  if (f11 > best) { best = f11; id = w11; }
  if (f21 > best) { best = f21; id = w21; }
  
  if (V) printf ("%d %d w0\t%.0f\t%.0f\t%s\t%s\n", w0==W, f0==best,  f0,  F[w0],  word, id2key(H,W));
  if (V) printf ("%d %d w1\t%.0f\t%.0f\t%s\n",     w1==W, f1==best,  f1,  F[w1],  id2key(H,w1));
  if (V) printf ("%d %d w11\t%.0f\t%.0f\t%s\n",   w11==W, f11==best, f11, F[w11], id2key(H,w11));
  if (V) printf ("%d %d w2\t%.0f\t%.0f\t%s\n",     w2==W, f2==best,  f2,  F[w2],  id2key(H,w2));
  if (V) printf ("%d %d w21\t%.0f\t%.0f\t%s\n",   w21==W, f21==best, f21, F[w21], id2key(H,w21));
  
  return id;
}



//////////////////////////////////////////////////////////////////////////////////////////

#ifdef MAIN

void qselect (ix_t *X, int k) ;
void print_vec_svm (ix_t *vec, hash_t *ids, char *vec_id, char *fmt);
void dedup_vec (ix_t *X);
float *vec2full (ix_t *vec, uint N, float def);
uint num_cols (coll_t *c) ;

int do_edits_1 (char *word, uint nedits, hash_t *known, float *cf) {
  ulong t0, t1, t2, t3, t4;
  hash_t *all = open_hash(0,0);
  t0=ustime(); all_edits (word, nedits, all);
  t1=ustime(); ix_t *scored = score_edits (all, known, cf);
  t2=ustime(); qselect (scored, 10);
  t3=ustime(); sort_vec (scored, cmp_ix_X);
  t4=ustime(); print_vec_svm (scored, known, word, "%.0f");
  printf ("edits: %ld known: %ld top: %ld sort: %ld\n", t1-t0, t2-t1, t3-t2, t4-t3);
  free_hash(all);
  free_vec(scored);
  return 0;
}

int do_edits_2 (char *word, uint nedits, hash_t *known, float *cf) {
  ulong t0, t1, t2, t3, t4, t5;
  ix_t *scored = new_vec (0, sizeof(ix_t));
  t0=ustime(); known_edits (word, nedits, known, cf, &scored);
  t1=ustime(); sort_vec (scored, cmp_ix_i);
  t2=ustime(); dedup_vec (scored);
  t3=ustime(); qselect (scored, 10);
  t4=ustime(); sort_vec (scored, cmp_ix_X);
  t5=ustime(); print_vec_svm (scored, known, word, "%.0f");
  printf ("known: %ld sort: %ld dedup: %ld top: %ld sort: %ld\n", t1-t0, t2-t1, t3-t2, t4-t3, t5-t4);
  free_vec(scored);
  return 0;
}

int do_edits_3 (char *word, uint nedits, hash_t *known, float *cf) {
  ulong t0, t1; uint best = 0;
  t0=ustime(); best_edit (word, nedits, known, cf, &best);
  t1=ustime(); char *corr = best ? id2key(known,best) : "";
  printf ("%s %s:%.0f\n", word, corr, cf[best]);
  printf ("best: %ld\n", t1-t0);
  return 0;
}

int do_edits (char *word, uint n, char *_H, char *_F) {
  hash_t *H = open_hash (_H,"r");
  float *F = mtx_full_row (_F, 1);
  do_edits_3 (word, n, H, F); printf("----------\n");
  do_edits_2 (word, n, H, F); printf("----------\n");
  do_edits_1 (word, n, H, F); printf("----------\n");  
  free_vec(F);
  free_hash(H);
  return 0;
}

int do_fuse (char *w1, char *w2, char *_H, char *_F) {
  hash_t *H = open_hash (_H,"r");
  float *F = mtx_full_row (_F, 1);
  uint id = try_fuse (w1, w2, H);
  uint i1 = has_key(H,w1), i2 = has_key(H,w2);
  char *word = id ? id2key(H,id) : "N/A";
  printf("%s:%.0f <- %s:%.0f %s:%.0f\n",
	 word, F[id], w1, F[i1], w2, F[i2]);
  free_vec(F);
  free_hash(H);
  return 0;
}

int do_split (char *word, char *_H, char *_F) {
  hash_t *H = open_hash (_H,"r");
  float *F = mtx_full_row (_F, 1);  
  uint i1=0, i2=0;
  try_runon (word, H, F, &i1, &i2);
  char *w1 = i1 ? id2key(H,i1) : "N/A";
  char *w2 = i2 ? id2key(H,i2) : "N/A";
  uint id = has_key(H,word);
  printf("%s:%.0f -> %s:%.0f %s:%.0f\n", word, F[id], w1, F[i1], w2, F[i2]);
  free_vec(F);
  free_hash(H);
  return 0;
}

int do_pubmed (char *word, char *_H, char *_F, char *prm) {
  char *pickmax = strstr(prm,"pickmax");
  hash_t *H = open_hash (_H,"r");
  float *F = mtx_full_row (_F, 1);
  uint i0 = has_key(H,word), id, id2 = 0;
  if (pickmax) id = pickmax_spell (word, H, F, prm, nkeys(H)+2);
  else         id =  pubmed_spell (word, H, F, prm, nkeys(H)+2, &id2);
  printf("%s:%.0f -> %s:%.0f", word, F[i0], id2key(H,id), F[id]);
  if (id2) printf(" + %s:%.0f", id2key(H,id2), F[id2]);
  puts("");
  free_vec(F);
  free_hash(H);
  return 0;
}

int do_eval_spell (char *_H, char *_F, char *prm) {
  char *pickmax = strstr(prm,"pickmax");
  char V = !strstr(prm,"silent");
  hash_t *H = open_hash (_H,"r");
  float *F = mtx_full_row (_F, 1);
  char trg[1000], *src=0, *eol=0;
  double t0 = ftime(), n = 0, oks = 0;
  while (fgets (trg, 999, stdin)) {
    if ((eol = index(trg,'\n'))) *eol = 0;
    if ((src = index(trg,'\t'))) *src++ = 0;
    uint it = has_key(H,trg), is = has_key(H,src), id = 0, ok = 0;
    if (it) {
      if (pickmax) id = pickmax_spell (src, H, F, prm, it);
      else         id =  pubmed_spell (src, H, F, prm, it, 0);
      ok = (id == it);
      if (V) printf("EVAL %d %s:%.0f -> %s:%.0f %s:%.0f\n",
		    ok, src, F[is], id2key(H,id), F[id], trg, F[it]);
      oks += ok; ++n;
    }
  }
  double lag = (ftime()-t0), acc = oks/n;
  printf ("%.4f %.2fs %s\n", acc, lag, prm);
  free_vec(F);
  free_hash(H);
  return 0;
}

int do_levenstein (char *A, char *B) {
  char buf[999]; uint d = levenstein_distance (A, B, buf);
  printf ("%s -(%d)-> %s: %s\n", A, d, B, buf);
  return 0;
}

#define arg(i) ((i < argc) ? A[i] : NULL)
#define a(i) ((i < argc) ? A[i] : "")

char *usage =
  "usage: spell -edits remdesivir 1 WORD WORD_CF\n"
  "       spell -fuse  amino acid WORD WORD_CF\n"
  "       spell -split coronavirus WORD WORD_CF\n"
  "       spell -pub   coronavirus WORD WORD_CF [verbose]\n"
  "       spell -eval  WORD WORD_CF [quiet] < pairs.tsv\n"
  "       spell -dist  word word\n"
  ;

int main (int argc, char *A[]) {
  if (argc < 2) return fprintf (stderr, "%s", usage);
  if (!strcmp(a(1),"-fuse"))  return do_fuse (a(2), a(3), a(4), a(5));
  if (!strcmp(a(1),"-split")) return do_split (a(2), a(3), a(4));
  if (!strcmp(a(1),"-edits")) return do_edits (a(2), atoi(a(3)), a(4), a(5));
  if (!strcmp(a(1),"-pub"))   return do_pubmed (a(2), a(3), a(4), a(5));
  if (!strcmp(a(1),"-eval"))  return do_eval_spell (a(2), a(3), a(4));
  if (!strcmp(a(1),"-dist"))  return do_levenstein (a(2), a(3));
  
  if (argc == 3) {
    hash_t *H = open_hash (0,0);
    double t0 = 1E3 * ftime();
    all_edits(A[1], atoi(A[2]), H);
    double t1 = 1E3 * ftime();
    uint n = nkeys(H);
    printf ("%.0fms: %d keys\n", t1-t0, n);
    //for (i = 1; i <= n; ++i) printf ("%d\t%s\n", i, id2key(H,i));  
    free_hash (H);
    return 0;
  }
  if (argc == 5) {
    do_edit (A[1], strlen(A[1]), atoi(A[2]), A[3][0], A[4][0]);
    printf ("'%s'\n", A[1]);
    return 0;
  }
  return 1;
}

#endif
