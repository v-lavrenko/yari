#include "hash.h"

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


// ---------------------------------------- v-byte encoding

//#define last_byte(char b) b & 

// in-place reverse byte array [beg..end) 
void reverse (char *beg, char *end) { 
  for (--end; beg < end; ++beg, --end) {
    char tmp = *beg; *beg = *end; *end = tmp;
  }
}

// push vbytes of U into buffer, return EOB
char *push_vbyte (uint U, char *buf) {
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
char *pop_vbyte (uint *U, char *buf) {
  *U = 0;
  do {
    *U <<= 7; // make space for 7 bits at the end of U
    *U |= (*buf & 127); // vbyte -> 7 lowest bits
    if (*buf++ & 128) break; // vbyte null-terminated
  } while(1);
  return buf; // new end of buffer
}

char *vbyte_encode (uint *U) {
  char *B = new_vec (4*len(U), sizeof(char)), *buf = B;
  uint *u = U-1, *end = U+len(U);
  while (++u < end) buf = push_vbyte (*u, buf);
  len(B) = buf - B;
  return B;
}

uint *vbyte_decode (char *B) {
  uint *U = new_vec (0, sizeof(uint)), u;
  char *b = B, *end = B+len(B);
  while (b < end) {
    b = pop_vbyte (&u, b);
    U = append_vec (U, &u);
  }
  return U;  
}

// ---------------------------------------- Elias Gamma

// prefix: z = ilog2(n) zeros ... ilog(42) = 5 -> 00000
// suffix: n - 2^z in binary  ... 42 - 32 = 10 -> 01010
// code: prefix1suffix        ... 42 -> 00000 1 01010


char *push_gamma (uint U, char *buf) {
  //char *end = buf-1;
  //uint pfx = ilog2 (U);
  //while (++end < buf+pfx) *end = '0';
  (void)U; return buf;  
}

char *pop_gamma (uint *U, char *buf) {
  (void)U; return buf;  
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
  while (++v < end) printf("%d ", *v);
  printf("[%d]\n", len(V)*vesize(V));
}

#define getbit(B,i) (B & (1 << (i & 31)))

void show_binary (char c) {
  int i;
  for (i=7; i>=0; --i) fputc((getbit(c,i)?'1':'0'), stdout);
  fputc(' ',stdout);
}

void show_buf (char *B, char *pre) {
  char *b = B-1, *end = B+len(B);
  if (pre) printf ("%s ", pre);
  //while (++b < end) printf("%hhx ", *b);
  while (++b < end) show_binary (*b);
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

int do_mtx_debug (uint i, uint *U, uint *V, char *B, char *pre) {
  printf ("row %d %s\n", i, pre);
  show_vec (U,"U:");
  show_vec (V,"V:");
  show_buf (B,"B:");
  return 1;
}

#define vsize(v) (len(v) * vesize(v) + sizeof(vec_t))

int do_mtx (char *_M) {
  coll_t *M = open_coll (_M, "r+");
  int i, n = nvecs(M);
  for (i = 1; i <= n; ++i) {
    ix_t *V = get_vec_ro (M, i);
    uint *U = ix2i (V), j;
    char *B = vbyte_encode (U);
    uint *BU= vbyte_decode (B);
    delta_encode (U);
    char *D = vbyte_encode (U);
    uint *DU= vbyte_decode (D);
    delta_decode (U);
    delta_decode (DU);
    double szU = vsize(U), szB = vsize(B), szD = vsize(D);
    double crB = 100*szB/szU, crD = 100*szD/szU;
    assert (len(U) == len(BU));
    assert (len(U) == len(DU));
    for (j=0; j<len(U); ++j) {
      if (U[j] != BU[j]) return do_mtx_debug (i, U, BU, B, "B");
      if (U[j] != DU[j]) return do_mtx_debug (i, U, DU, D, "D");
    }
    printf ("%4d vbyte: %.0f%% delta: %.0f%% %d\n", i, crB, crD, len(U)); fflush(stdout);
    free_vec (U); free_vec (B); free_vec (D); free_vec (BU); free_vec (DU);
  }
  free_coll (M);
  return 0;
}

char *usage =
  "compress -delta 1 2 3 4 5 6 7 8\n"
  "compress -vbyte 1 2 3 4 5 6 7 8\n"
  "compress -mtx ROWSxCOLS\n"
  ;

#define arg(i) ((i < argc) ? argv[i] : NULL)
#define a(i) ((i < argc) ? argv[i] : "")

int main (int argc, char *argv[]) {
  if (argc < 2) return fprintf (stderr, "%s", usage);
  if (!strcmp (a(1),"-delta")) return do_delta(argc-2,argv+2);  
  if (!strcmp (a(1),"-vbyte")) return do_vbyte(argc-2,argv+2);
  if (!strcmp (a(1),"-mtx")) return do_mtx(arg(2));
  return 1;
}

#endif
