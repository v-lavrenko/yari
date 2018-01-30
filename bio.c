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
#include "textutil.h"

static uint in_range (int i, uint min, uint max) {
  uint j = (i < 0) ? (max+i) : (uint)i;
  return (j < min) ? min : (j > max) ? max : j;
}

static it_t parse_slice (char *slice, uint min, uint max) {
  char *s = slice + strspn (slice,"[( "), *sep = index(s,':');
  int lo = atoi(s), hi = sep ? atoi(sep+1) : 0;
  it_t d = {min, max};
  if      (lo && hi)  d = (it_t) {lo, hi};
  else if (lo && sep) d = (it_t) {lo, max};
  else if (hi && sep) d = (it_t) {min, hi};
  else if (lo)        d = (it_t) {lo, lo};
  d.i = in_range (d.i, min, max);
  d.t = in_range (d.t, min, max);
  return d;
}

// TODO: compress 4 chars -> 1 byte + 1 byte footer
void load_fasta (char *SEQS) { // thread-unsafe: show_progress
  uint id = 0;
  char *buf = malloc(1<<30), *c;
  coll_t *C = open_coll (SEQS, "w+");   // id -> ATCGCGTCGGTTAAGGTCCATG
  while (fgets(buf,1<<30,stdin)) {
    if (*buf == ';' || *buf == '>') continue; // skip FASTA comments
    for (c = buf; *c; ++c) *c = toupper(*c); // uppercase
    if (c > buf && c[-1] == '\n') c[-1] = 0; // remove newline
    put_chunk (C, ++id, buf, strlen(buf)+1);
    if (0==id%10000) show_progress (id, 0, "chunks");
  }
  fprintf (stderr, "%s: %d chunks\n", SEQS, id);
  free_coll (C); free (buf);
}

#define BIT(x,n) ((x<<(32-n))>>(31))
#define ROT(x,n) ((x<<n)|(x>>(32-n)))

char bits2c (uint b) { return (b==0) ? 'A' : (b==1) ? 'C' : (b==2) ? 'G' : 'T'; }
uint c2bits (char c) { return (c=='A') ? 0 : (c=='C') ? 1 : (c=='G') ? 2 : 3; }
uint c2rand (char c) { return ((c=='A') ? 0xc73ee513 : (c=='C') ? 0x3a5e13de : 
				      (c=='G') ? 0xf67cc174 :            0x01ca1a35); }

uint tucode (uint i, uint j) { return ROT(i,16) ^ j; }

void bits2kmer (uint bits, char *mer, uint k) {
  uint i; mer[k] = 0;
  for (i = 0; i < k; ++i) { mer[i] = bits2c (bits&3); bits >>= 2; }
}

uint kmer2bits (char *mer, uint k) {
  uint bits = 0, i;
  for (i = 0; i < k; ++i) { bits = (bits << 2) | c2bits (mer[i]); }
  return bits;
}

uint update_kmer (uint bits, uint k, char add) {
  uint truncate = (1LL << (k << 1)) - 1; // 2k lowest bits
  return ((bits << 2) | c2bits(add)) & truncate;
} // L-shift old bits by 2, append 2 new bits, truncate to 2k

uint update_buzz (uint bits, uint k, char add, char del) {
  uint head = c2rand(add), tail = c2rand(del);
  return ROT(bits,1) ^ head ^ ROT(tail,k); // buzhash
}

ix_t *seq2pos (char *seq, uint k) {
  uint code = 0, i, n = strlen(seq);
  assert (n > k);
  ix_t *pos = new_vec (n, sizeof(ix_t)), *p = pos;
  for (i = 0; i < n; ++i) {
    //code = update_buzz (code, k, seq[i], /* FIX >>> */ seq[i-k]); // <<< FIX
    code = update_kmer (code, k, seq[i]); 
    if (i >= k-1) *p++ = (ix_t) {code+1, i-k+2}; // +1 to avoid zero
  }
  len(pos) = p-pos;
  return pos;
}

ix_t *seq2posD (char *seq, uint k, hash_t *D) {
  uint i, n = strlen(seq) - k + 1;
  ix_t *pos = new_vec (n, sizeof(ix_t));
  for (i = 0; i < n; ++i) {
    pos[i].x = i+1; // position, starting from 1
    pos[i].i = key2id (D, substr (seq+i,k)); // k-gram starting at i
  }
  return pos;
}

void seqs2kmer (char *_CODE, char *_SEQS, char *prm) { // thread-unsafe: show_progress
  coll_t *SEQS = open_coll (_SEQS, "r+");  // id -> ATCGAGTCGGTTAAGGTCCATG
  coll_t *CODE = open_coll (_CODE, "w+");  // id -> ATCG:1 TCGA:2 CGAG:3 ...
  uint i, ns = num_rows (SEQS), k = getprm(prm,"k=",10);
  char *nosort = strstr(prm,"nosort");
  char *uniq = strstr(prm,"uniq");
  char *dict = getprmp (prm, "dict=", NULL);
  hash_t *DICT = dict ? open_hash (dict, "w") : NULL;
  for (i = 1; i <= ns; ++i) {
    char *seq = get_chunk (SEQS, i);
    ix_t *pos = DICT ? seq2posD (seq, k, DICT) : seq2pos (seq, k);
    if (!nosort) sort_vec (pos, cmp_ix_i);
    if (uniq) { vec_x_num (pos,'=',1); uniq_vec (pos); }
    put_vec (CODE, i, pos);
    free_vec (pos);
    if (0==i%1000) show_progress (i, ns, "vecs");
  }
  free_coll (SEQS); free_coll (CODE); free_hash (DICT);
}

void pos2freq (ix_t *pos) { // [ (id,pos) ] -> [ (id,freq) ]
  vec_x_num (pos, '=', 1);
  sort_vec (pos, cmp_ix_i);
  uniq_vec (pos);
}

ix_t *pos2loc (ix_t *P, uint N) { // [ (id,pos) ] -> [ (idpos,freq) ]
  uint r, R = sqrt(N); // default replication
  ix_t *loc = const_vec (N*R, 1), *l = loc, *p;
  for (p = P; p < P+N; ++p) 
    for (r = 0; r < R; ++r) { // replicate (blur) position R times
      uint id = p->i, pos = p->x - P->x + r;
      l->i = ROT(id,24) ^ ROT(pos,8); // shift-XOR: id + position
      ++l;
    }
  sort_vec (loc, cmp_ix_i);
  uniq_vec (loc);
  return loc;
}

ix_t *pos2lsh (ix_t *pos, char *prm) {
  uint N = getprm(prm,"N=",100); // subsequence size
  uint B = getprm(prm,"B=",20);  // bits per table (probe)
  uint T = getprm(prm,"T=",10);  // tables (probes per position)
  ix_t *result = new_vec (0, sizeof(ix_t)), *p;
  sort_vec (pos, cmp_ix_x);      // must order by position
  for (p = pos; p < pos + len(pos) - N; ++p) {
    ix_t *locs = pos2loc (p,N);
    ix_t *bits = simhash (locs, B*T, "Bernoulli");
    ix_t *code = bits2codes (bits, T);
    vec_x_num (code, '=', p->x); // set position to p
    result = append_many (result, code, T);
    free_vec (locs); free_vec (bits); free_vec (code);
  }
  sort_vec (result, cmp_ix_i);
  return result;
}

void mtx2mtx (char *_TRG, char *_SRC, char *prm) { // thread-unsafe: show_progress
  coll_t *SRC = open_coll (_SRC, "r+");
  coll_t *TRG = open_coll (_TRG, "w+");
  char *freq = strstr(prm,"freq");
  char *locs = strstr(prm,"locs");
  char *plsh = strstr(prm,"plsh");
  uint i, n = num_rows (SRC);
  for (i = 1; i <= n; ++i) {
    ix_t *S = get_vec (SRC, i), *T;
    if      (plsh) T = pos2lsh (S, prm);
    else if (locs) T = pos2loc (S, len(S));
    else if (freq) pos2freq (T = copy_vec(S));
    else           T = copy_vec (S);
    put_vec (TRG, i, T);
    free_vec (S); free_vec (T);
    if (0==i%1000) show_progress (i, n, "vecs");
  }
  free_coll (SRC); free_coll (TRG); 
}


void deltas (char *_DELTA, char *_CODES, char *_INVLS, char *prm) {
  (void) prm;
  coll_t *DELTA = open_coll (_DELTA, "w+");   // chunk -> [ chunk:delta ]
  coll_t *CODES = open_coll (_CODES, "r+");   // chunk -> [ code:offset ]
  coll_t *INVLS = open_coll (_INVLS, "r+");   // code -> [ chunk:offset ]
  uint i, nc = num_rows (CODES);
  
  for (i=1; i<=nc; ++i) { // for each chunk 
    ix_t *D = new_vec (0, sizeof(ix_t));
    ix_t *C = get_vec (CODES, i), *c;
    for (c = C; c < C+len(C); ++c) { // for each code in chunk
      ix_t *L = get_vec (INVLS, c->i), *l; // chunks matching code
      for (l = L; l < L+len(L); ++l) if (l->i == i) l->i = 0;
      chop_vec (L); // remove chunk matching itself (l->i == i)
      for (l = L; l < L+len(L); ++l) l->x = c->x - l->x; // delta
      D = append_many (D, L, len(L)); // D = concatenation of all L
      free_vec (L);
    }
    sort_vec (D, cmp_ix_i);
    put_vec (DELTA, i, D);
    free_vec (D); free_vec (C);
  }
  free_coll(DELTA); free_coll(CODES); free_coll(INVLS);
}

void graph_shift (char *_NEW, char *_POS, char *_DELTA, char *prm) {
  (void) prm;
  coll_t *N = open_coll (_NEW, "w+");    // result: new distribution
  coll_t *P = open_coll (_POS, "r+");    // distribution of positions per chunk
  coll_t *D = open_coll (_DELTA, "r+");  // deltas between pairs of chunks 
  uint j, n = num_rows (D);
  
  for (j = 1; j <= n; ++j) {
    ix_t *Nj = get_vec (P,j), *tmp=0;      // P[j] -> [ pos1:conf, pos2:conf ]
    ix_t *Dj = get_vec (D,j), *d;          // D[j] -> [ i1:d(j,i1), i2:d(j,i2) ]
    for (d = Dj; d < Dj+len(Dj); ++d) {
      ix_t *Pi = get_vec (P,d->i), *p;     // P[i] -> [ pos1:conf, pos2:conf ]
      for (p = Pi; p < Pi+len(Pi); ++p) p->i -= d->x; // pos[j] = pos[i] - d(j,i)
      Nj = vec_add_vec (1, tmp=Nj, 1, Pi); // replace with TAAT if P or D dense
      free_vec (Pi); free_vec (tmp);
    }
    put_vec (N,j,Nj);
    free_vec (Nj); free_vec (Dj);
  }
  free_coll(N); free_coll(P); free_coll(D);
}

uint edits (char *A, char *B) ;

void mtx_distance (char *_A, char *_B, char *prm) {
  coll_t *AA = open_coll (_A, "r+");
  coll_t *BB = open_coll (_B, "r+");
  char *edit = strstr(prm,"edit");
  char *norm = strstr(prm,"norm");
  float p = getprm (prm, "p=", 1), d = 0;
  uint nA = num_rows(AA), nB = num_rows(BB); assert (nA == nB);
  uint i, n = getprm (prm, "n=", nA);
  for (i = 1; i <= n; ++i) {
    if (edit) {
      char *A = get_chunk(AA,i), *B = get_chunk(BB,i);
      d = edits (A, B);
      if (norm) d /= strlen(A);
    } else {
      ix_t *A = get_vec (AA,i), *B = get_vec (BB,i);
      d = pnorm (p, A, B);
      if (norm) d /= sum(A) + sum(B);
      free_vec (A); free_vec (B);
    }
    printf ("%d %.6f\n", i, d);
  }
  free_coll(AA); free_coll(BB); 
}

//---------------------------------------------------------------------- printing

/*
void dump_vecs (char *VECS, char *_rows) {
  coll_t *C = open_coll (VECS, "r+");
  it_t rows = parse_slice (_rows, 1, num_rows(C));
  uint id;
  for (id = rows.i; id <= rows.t; ++id) {
    ix_t *V = get_vec (C,id), *v;
    for (v = V; v < V+len(V); ++v) {
    }
  }
  free_coll (C); 
}
*/

void dump_fasta (char *SEQS, char *_rows, char *_cols) {
  coll_t *C = open_coll (SEQS, "r+");   // id -> ATCGCGTC 
  it_t rows = parse_slice (_rows, 1, num_rows(C));
  it_t cols = parse_slice (_cols, 1, 1<<30);
  fprintf (stderr, "%s [%d:%d] [%d:%d]\n", SEQS, rows.i, rows.t, cols.i, cols.t);
  uint id;
  for (id = rows.i; id <= rows.t; ++id) {
    char *seq = get_chunk(C,id);
    it_t cols = parse_slice (_cols, 1, strlen(seq));
    printf ("%.*s\n", (cols.t-cols.i+1), seq + cols.i - 1);
  }
  free_coll (C); 
}


void dump_align (char *_SEQS, char *_POSS) {
  coll_t *SEQS = open_coll (_SEQS, "r+");  //
  coll_t *POSS = open_coll (_POSS, "r+");  //
  jix_t *P = max_rows (POSS), *L=P, *p; // [{seq,pos,wt} ]
  for (p = P; p < P+len(P); ++p) if (p->i < L->i) L = p; // leftmost sequence
  for (p = P; p < P+len(P); ++p) {
    char *seq = get_chunk (SEQS,p->j);
    printf ("%*s", (p->i - L->i), "");
    printf ("%s\n", seq);
  }
  free_coll (SEQS); free_coll (POSS); free_vec (P);
}

//---------------------------------------------------------------------- Levenstein

//#define REF(A,w,r,c) (A[
void levenstein (char *A, int NA, char *B, int NB, int w) {
  int a, i, W = 2*w+1, inf = NA+NB, nA = NA-1, nB = NB-1;
  int *E = calloc (W, sizeof(int));
  char *O = calloc (W, nA+1);
  for (a = 0; a <= nA; ++a) { // for each row
    for (i = 0; i < W; ++i) {
      int b = a-w+i ;
      if (b >= 0 && b <= nB) {
	int D = (a == 0 ? b+1 :                  E[i])   + (A[a] != B[b]);
	int U = (a == 0 ? b+1 : i == W-1 ? inf : E[i+1]) + 1;
	int L = (b == 0 ? a+1 : i == 0   ? inf : E[i-1]) + 1;
	if      (U < MIN(L,D)) { E[i] = U;   O[a*W+i] = 'U'; }
	else if (L < MIN(U,D)) { E[i] = L;   O[a*W+i] = 'L'; }
	else                   { E[i] = D;   O[a*W+i] = 'D'; }
      } else                   { E[i] = inf; O[a*W+i] = '.'; }
      printf (" %2d%c", E[i],O[a*W+i]);
    }
    printf ("\n");
  }
  //  while (a >= 0 && b >= 0) {
  //    *--out = O[a*W+i];
  //    if      (O[a*W+i] == 'D') --a;
  //    else if (O[a*W+i] == 'L') --i;
  //    else {--a; ++i;}
  //  }
}

uint edits (char *A, char *B) {
  uint a, b, nA = strlen(A), nB = strlen(B); assert (nA >= nB);
  uint *E = calloc (nA+1, sizeof(uint));
  uint *F = calloc (nA+1, sizeof(uint)), ed = 0;
  for (a = 0; a <= nA; ++a) E[a] = a;
  for (b = 1; b <= nB; ++b) {
    F[0] = b;
    for (a = 1; a <= nA; ++a) {
      uint diag = E[a-1] + (A[a-1] == B[b-1] ? 0 : 1);
      uint   up = E[a] + 1;
      uint left = F[a-1] + 1;
      ed = MIN(up,left);
      ed = MIN(ed,diag);
      F[a] = ed;
    }
    for (a = 0; a <= nA; ++a) E[a] = F[a];
  }
  free (E); free (F); 
  return ed;
}

//---------------------------------------------------------------------- MAIN

char *usage = 
  "bio S = load           - read FASTA sequences from stdin\n"
  "bio C = kmer:[prm] S   - convert seqs to positional k-mers\n"
  "                         prm: k=10 - default k-mer size\n"
  "                              uniq - frequencies instead of positions\n"
  "                              nosort - keep ordered by position\n"
  "                              dict=H - maps string <-> id\n"
  "bio C = freq C         - convert positions to frequencies\n"
  "bio C = locs C         - convert positional to locus features\n"
  "bio C = plsh C         - convert positional features to positional LSH\n"
  "bio D = C delta C.T    - enumerate deltas from C [chunks x codes] to C.T [codes x chunks]\n"
  "bio Q = P shift D      - shift positions P [chunks x pos] using deltas D [chunks x chunks]\n"
  "bio dump S [1:N] [1:M] - print characters 1..M from sequences 1..N\n"
  "bio dist:[prm] A B     - pairwise distances: D(A[i],B[i])\n"
  "                         prm: p=1 - p-norm (0:Hamming, 1:Manhattan, 2:Euclidean)\n"
  "                              edits - Levenstein (A,B: sequence collections)\n"
  ;

#define a(i) ((i < argc) ? argv[i] : "")

int main (int argc, char *argv[]) {
  //levenstein ("abcdefgh",8,"bcdaegh",7,2); return 0;
  if (argc < 3) { printf ("%s", usage); return 1; }
  if (!strcmp(a(2),"=") && argc > 3) {
    char *tmp = calloc(1000,1);
    sprintf (tmp, "%s.%d", a(1), getpid()); // temporary target
    if      (!strncmp (a(3), "load", 4)) load_fasta (tmp);
    else if (!strncmp (a(3), "kmer", 4)) seqs2kmer (tmp, a(4), a(3));
    else if (!strncmp (a(3), "freq", 4) || 
	     !strncmp (a(3), "locs", 4) || 
	     !strncmp (a(3), "plsh", 4)) mtx2mtx (tmp, a(4), a(3));
    else if (!strncmp (a(4), "shift",5)) graph_shift (tmp, a(3), a(5), a(4));
    else if (!strncmp (a(4), "delta",5)) deltas (tmp, a(3), a(5), a(4));
    mv_dir (tmp, a(1));
    free(tmp);
  }
  else if   (!strncmp (a(1), "dump:align", 10)) dump_align (a(2), a(3));
  else if   (!strncmp (a(1), "dump", 4)) dump_fasta (a(2), a(3), a(4));
  else if   (!strncmp (a(1), "dist",4)) mtx_distance (a(2), a(3), a(1));
  return 0;
}
