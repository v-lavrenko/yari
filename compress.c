#include "hash.h"
#include "bitvec.h"
#include "timeutil.h"

/* -------------------- ideas -------------------- 
   
   general:
   (a) separate components: [{i1,x2} ... {iN,xN}] -> [i1..iN] [x1..xN]
   (b) [i1..iN]: delta + v-byte compression 
   (c) [x1..xN]: sort | uniq -c | sort -rn -> [j1..jN] + MAP:j->x
   (d) [j1..jN]: v-byte or Huffman 
   (e) MAP[0] = default (zero) value?
   (f) MAP based on entire matrix or one vec?
   
   (g) if N > NC/2 -> store full vec [x1..xNC]
   (h) if x = const -> store [i1..iN] + const
   
   (i) allow MAP:j->offset into STRINGS[]
   
   ----------------------------------------------- */


// ---------------------------------------- delta encoding

void delta_encode (uint *V) {
  uint i = len(V);
  while (--i > 0) V[i] -= V[i-1];
}

void delta_decode (uint *V) {
  uint n = len(V), i=0;
  while (++i < n) V[i] += V[i-1];
}


// ---------------------------------------- v-byte / varint encoding

//#define last_byte(char b) b & 

// in-place reverse byte array [beg..end) 
void reverse (char *beg, char *end) { 
  for (--end; beg < end; ++beg, --end) {
    char tmp = *beg; *beg = *end; *end = tmp;
  }
}

// push vbytes of U into buffer, return EOB
char *push_vbytes (uint U, char *buf) {
  char *end = buf;
  do {
    *end++ = U & 127; // 7 lowest bits (vbyte) -> end of buffer
    U >>= 7; // shift out 7 lowest bits
  } while (U); // nothing to encode
  reverse (buf, end); // most-significant first
  end[-1] |= 128; // terminate last vbyte
  return end; // new end of buffer
}

// pop vbytes from buf, reconstruct U, return EOB
char *pop_vbytes (uint *U, char *buf) {
  *U = 0;
  do {
    *U <<= 7; // make space for 7 bits at the end of U
    *U |= (*buf & 127); // vbyte -> 7 lowest bits
    if (*buf++ & 128) break; // vbyte null-terminated
  } while(1);
  return buf; // new end of buffer
}

char *vbyte_encode (uint *U) {
  char *B = new_vec (5*len(U), sizeof(char)), *buf = B;
  uint *u = U-1, *end = U+len(U);
  while (++u < end) buf = push_vbytes (*u, buf);
  len(B) = buf - B;
  return B;
}

uint *vbyte_decode (char *B) {
  uint *U = new_vec (0, sizeof(uint)), u = 0;
  char *b = B, *end = B+len(B);
  while (b < end) {
    b = pop_vbytes (&u, b);
    U = append_vec (U, &u);
  }
  return U;
}

// Little Endian: 0x11223344 -> {44,33,22,11} in memory

// ---------------------------------------- https://www.slideshare.net/profyclub_ru/sphinx-82002790

uchar *push_msint (uint U, uchar *B) {
  uint *u = (uint*) (B+1);
  if (U <= 251)      { *B = U; return B+1; }
  if (U <= 0xff)     { *B = 252; *u = U & 0xff; return B+2; }
  if (U <= 0xffff)   { *B = 253; *u = U & 0xffff; return B+3; }
  if (U <= 0xffffff) { *B = 254; *u = U & 0xffffff; return B+4; }
  {                    *B = 255; *u = U;              return B+5; }  
}

uchar *pop_msint (uint *U, uchar *B) {
  uint *u = (uint*) (B+1);
  switch (*B) { // 1st byte
  case 252: *U = *u & 0xff;     return B+2;
  case 253: *U = *u & 0xffff;   return B+3;
  case 254: *U = *u & 0xffffff; return B+4;
  case 255: *U = *u;            return B+5;
  default:  *U = *B;            return B+1;
  }
}

char *msint_encode (uint *U) {
  uchar *B = new_vec (5*len(U), sizeof(char)), *buf = B;
  uint *u = U-1, *end = U+len(U);
  while (++u < end) buf = push_msint (*u, buf);
  len(B) = buf - B;
  return (char*)B;
}

uint *msint_decode (char *B) {
  uint *U = new_vec (0, sizeof(uint)), u=0;
  uchar *b = (uchar*)B, *end = b+len(b);
  while (b < end) {
    b = pop_msint (&u, b);
    U = append_vec (U, &u);
  }
  return U;
}

// ---------------------------------------- Elias Gamma

// little endian: 0x11223344 -> {44,33,22,11} in memory

// prefix: ilog2(n) zeros ... 00000 <- 5 = ilog(42)
// suffix: n in binary    ... 101010 <- 42
// code: prefix.suffix    ... 00000 101010 <- 42

void show_0xb (uint u) {
  int i = 31;
  for (; i>=24; --i) { fputc(((u & (1<<i)) ? '1' : '0'), stdout); } fputc(' ', stdout);
  for (; i>=16; --i) { fputc(((u & (1<<i)) ? '1' : '0'), stdout); } fputc(' ', stdout);
  for (; i>=8; --i)  { fputc(((u & (1<<i)) ? '1' : '0'), stdout); } fputc(' ', stdout);
  for (; i>=0; --i)  { fputc(((u & (1<<i)) ? '1' : '0'), stdout); } fputc(' ', stdout);
}

void show_buf0 (char *B) ;

off_t push_gamma (uint U, uchar *B, off_t beg) { // assume B[beg..] is zeroed out
  uint i, nb = ilog2(U)+1; // how many bits in binary repr. of U
  //printf ("gamma: %u | bits: %u | beg: %lu\n", U, nb, beg);
  beg += nb-1; // hop over prefix: nb-1 zeros
  for (i=0; i<nb; ++i) {
    uint j = nb-i-1;
    //show_0xb (U); printf (" bit [%d] -> %d\n", j, ((U & (1<<j))>0));
    if (U & (1<<j)) {
      //printf ("set %u -> %lu\n", j, beg+i);
      bit_set_1(B,(beg+i));
    }
  }
  return beg+nb;
}
  
off_t pop_gamma (uint *_U, uchar *B, off_t beg, off_t eob) {
  uint U = 0;
  off_t j, i = beg;  
  while ((i < eob) && !bit_is_1(B,i)) ++i;
  //printf ("popgamma %lu:%lu from ", beg, eob);
  //show_buf0((char*)B); 
  if (i >= eob) { /* printf(" i=%lu hit EOB, stop\n", i); */ return i; }
  uint nb = i - beg + 1; // how many bits I need
  //printf (" expect %u bits in %lu:%lu\n", nb, i, i+nb);
  for (j = i; j < i+nb; ++j) {
    char bit = (bit_is_1(B,j)) > 0;
    uint k = nb-(j-i)-1;
    if (bit) U |= (1 << k);
    //printf ("\tbit %lu is %d -> bit %u yields %u\n", j, bit, k, U);
  }
  //printf("\tfinal %u, new offset: %lu\n", U, j);
  *_U = U;
  return j;
}

char *gamma_encode (uint *U) {
  uchar *B = bit_vec (65*len(U)); off_t b = 0;
  uint *u = U-1, *end = U+len(U);
  while (++u < end) {
    //printf("gamma: %u -> B[%lu]\n", *u, b);
    b = push_gamma (*u, B, b);
  }
  //printf("trunc: %lu bits -> %lu bytes\n", b, ((b>>3)+1));
  len(B) = (b>>3) + 1;
  return (char*)B;
}

uint *gamma_decode (char *_B) {
  uint *U = new_vec (0, sizeof(uint)), u=0;
  uchar *B = (uchar*)_B;
  off_t b = 0, eob = len(B) << 3;
  while (b < eob) {
    b = pop_gamma (&u, B, b, eob);
    if (b < eob) U = append_vec (U, &u);
  }
  return U;
}

// ---------------------------------------- counting values

int is_constant (float *X) {
  float *x = X, *end = X + len(X);
  while (++x < end) if (*x != *(x-1)) return 0;
  return 1;
}

float *k_constants (float *X, uint K) {
  float *C = new_vec (K, sizeof(float));
  uint i, n = len(X), j, k = 0;
  for (i = 0; i < n; ++i) {
    for (j = 0; j < k && X[i] != C[j]; ++j);
    if (j == k) C[k++] = X[i];
  }
  return NULL;
}

ix_t *count_values (float *X) {
  uint n = len(X);
  ix_t *V = new_vec (n, sizeof(ix_t)), *v, *u, *end = V+n;
  for (v = V; v < end; ++v) { v->x = X[v-V]; v->i = 1; }
  sort_vec (V, cmp_ix_x); // by increasing value
  for (u = v = V; v < end; ++v) {
    if (u == v) continue;
    if (u->x == v->x) u->i += 1; // u->x >= v->x - eps
    else if (++u < v) *u = *v;
  }
  len(V) = u - V + 1;
  sort_vec (V, cmp_ix_I); // by decreasing count
  return V;
}

// ---------------------------------------- MAIN

#ifdef MAIN

uint *ix2i (ix_t *V) {
  uint i, n = len(V), *U = new_vec (n, sizeof(uint));
  for (i=0; i<n; ++i) U[i] = V[i].i;
  return U;  
}

uint *argv2vec (char *A[], int n) {
  uint *V = new_vec (n, sizeof(uint));
  while (--n >= 0) V[n] = atoi(A[n]);
  return V;
}

void show_vec (uint *V, char *pre) {
  uint *v = V-1, *end = V+len(V);
  if (pre) printf ("%s ", pre);
  while (++v < end) printf("%u ", *v);
  printf("[%u]\n", len(V)*vesize(V));
}

#define _getbit(B,i) (B & (1 << (i & 31)))
 
void show_binary (char c) {
  int i;
  for (i=7; i>=0; --i) fputc( ((c & (1<<i)) ? '1' : '0'), stdout);
  fputc(' ',stdout);
}

void show_buf0 (char *B) {
  char *b = B-1, *end = B+len(B);
  while (++b < end) show_binary (*b);  
}

void show_buf (char *B, char *pre) {
  if (pre) printf ("%s ", pre);
  show_buf0(B);
  printf("[%d]\n", len(B)*vesize(B));
}

int do_delta (int n, char *A[]) {
  uint *V = argv2vec (A,n);
  show_vec (V, "original:");
  delta_encode (V);
  show_vec (V, "encoded:");
  delta_decode (V);
  show_vec (V, "decoded:");
  free_vec(V); 
  return 0;
}

int do_vbyte (int n, char *A[]) {
  uint *V = argv2vec (A,n);
  show_vec (V, "original:");
  char *B = vbyte_encode (V);
  show_buf (B, "encoded:");
  uint *D = vbyte_decode (B);
  show_vec (D, "decoded:");
  free_vec(V); free_vec(B); free_vec(D);
  return 0;
}

int do_msint (int n, char *A[]) {
  uint *V = argv2vec (A,n);
  show_vec (V, "original:");
  char *B = msint_encode (V);
  show_buf (B, "encoded:");
  uint *D = msint_decode (B);
  show_vec (D, "decoded:");
  free_vec(V); free_vec(B); free_vec(D);
  return 0;
}

int do_gamma (int n, char *A[]) {
  uint *V = argv2vec (A,n);
  show_vec (V, "original:");
  char *B = gamma_encode (V);
  show_buf (B, "encoded:");
  uint *D = gamma_decode (B);
  show_vec (D, "decoded:");
  free_vec(V); free_vec(B); free_vec(D);
  return 0;
}

int do_mtx_debug (uint i, uint *U, uint *V, char *B, char *pre) {
  printf ("row %d %s\n", i, pre);
  show_vec (U,"U:");
  show_vec (V,"V:");
  show_buf (B,"B:");
  return 1;
}

#define vsize(v) (len(v) * vesize(v) + sizeof(vec_t))

char *do_encode (uint *U, char a) {
  switch (a) {
  case 'v': return vbyte_encode(U);
  case 'm': return msint_encode(U);
  case 'g': return gamma_encode(U);
  default:  return NULL;
  }
}

uint *do_decode (char *B, char a) {
  switch (a) {
  case 'v': return vbyte_decode(B);
  case 'm': return msint_decode(B);
  case 'g': return gamma_decode(B);
  default:  return NULL;
  }
}

#define ENCODE gamma_encode
#define DECODE gamma_decode

int do_mtx (char *_M, char *alg) {
  coll_t *M = open_coll (_M, "r+");
  int i, n = nvecs(M);
  double SZU = 0, SZB = 0, t0 = ftime();
  for (i = 1; i <= n; ++i) {
    uint *U = ix2i (get_vec_ro(M,i)), *V = copy_vec(U), j;
    delta_encode (V);
    char *B = do_encode (V, *alg);
    uint *BU= do_decode (B, *alg);
    delta_decode (BU);
    double szU = vsize(U), szB = vsize(B), crB = 100*szB/szU;
    double CRB = 100*(SZB+=szB)/(SZU+=szU);
    double dT = ftime() - t0, MpS = (SZB/dT)/1E6;
    assert (len(U) == len(BU));
    for (j=0; j<len(U); ++j) 
      if (U[j] != BU[j]) return do_mtx_debug (i, U, BU, B, "B");
    printf ("%d\t%d\t%s: %2.0f%% | %2.0f%% %.0f M/s\n",
	    i, len(U), alg, crB, CRB, MpS); fflush(stdout);
    free_vec (U); free_vec(V); free_vec (B); free_vec (BU); 
  }
  free_coll (M);
  return 0;
}

char *usage =
  "compress -delta 1 2 3 4 5 6 7 8\n"
  "compress -vbyte 1 2 3 4 5 6 7 8\n"
  "compress -msint 1 2 3 4 5 6 7 8\n"
  "compress -gamma 1 2 3 4 5 6 7 8\n"
  "compress -mtx ROWSxCOLS {vbyte,msint,gamma}\n"
  ;

#define arg(i) ((i < argc) ? argv[i] : NULL)
#define a(i) ((i < argc) ? argv[i] : "")

int main (int argc, char *argv[]) {
  if (argc < 2) return fprintf (stderr, "%s", usage);
  if (!strcmp (a(1),"-delta")) return do_delta(argc-2,argv+2);  
  if (!strcmp (a(1),"-vbyte")) return do_vbyte(argc-2,argv+2);
  if (!strcmp (a(1),"-msint")) return do_msint(argc-2,argv+2);
  if (!strcmp (a(1),"-gamma")) return do_gamma(argc-2,argv+2);
  if (!strcmp (a(1),"-mtx")) return do_mtx(arg(2), a(3));
  return 1;
}

#endif
