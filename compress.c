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
  uint i = len(V); if (!i) return;
  while (--i > 0) V[i] -= V[i-1];
}

void delta_decode (uint *V) {
  uint n = len(V), i=0;
  while (++i < n) V[i] += V[i-1];
}

// ---------------------------------------- move-to-front (MTF)

uint *mtf_encode (uint *U) { (void)U;
  return NULL;
}

uint *mtf_decode (uint *U) { (void)U;
  return NULL;
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

// ---------------------------------------- nibble (mysql-like)

void show_bufaz (uchar *B, uint a, uint z) ;

// Little Endian: 0xAaBbCcDd -> {Dd,Cc,Bb,Aa} in memory

//   1..8 -> 0xxx 
//   9..? -> 1nnn 1-8 nibbles follow

uint get_B_nibble (uchar *B, off_t n) {
  uint byte = n>>1, left = !(n&1), shift = left<<2;
  return 0xF & (B[byte] >> shift); }

void set_B_nibble (uchar *B, off_t n, uint u) {
  uint byte = n>>1, left = !(n&1), shift = left<<2;
  //printf ("u:%u byte:%u left:%u shift:%u ", u, byte, left, shift);
  B[byte] |= ((0xF & u) << shift); }

uchar get_U_nibble (uint U, uint n)          { return 0xF & (U >> ((7-n)<<2)); }
void  set_U_nibble (uint U, uint n, uchar u) { U |= ((0xF & u) << ((7-n)<<2)); }

// encode U -> B[b..e], return e, fail if e>=eob
off_t push_nibble (uint U, uchar *B, off_t b, uint eob) { (void)eob;
  if (U<8) {
    set_B_nibble (B, b, U);
    //printf ("push solo %u -> %lu | ", U, b); show_bufaz(B,0,1+b/2); puts("");
    return b+1;
  }
  int nibs = ((U<=0xF) ? 1 : (U<=0xFF) ? 2 : (U<=0xFFF) ? 3 : (U<=0xFFFF) ? 4 :
	      (U<=0xFFFFF) ? 5 : (U<=0xFFFFFF) ? 6 : (U<=0xFFFFFFF) ? 7 : 8), i;
  set_B_nibble (B, b, ((nibs-1)|8));
  //printf ("push header (%d nibs) -> %lu | ", nibs, b); show_bufaz(B,0,1+b/2); puts("");
  for (i = 1; i <= nibs; ++i) {
    uint u = 0xF & (U >> ((nibs-i) << 2));
    set_B_nibble (B, b+i, u);
    //printf ("push nibble %d: %u -> %lu | ", nibs-i, u, b+i); show_bufaz(B,0,(b+i)/2+1); puts("");
  }
  return b+nibs+1;
}

// decode B[b..e] -> *U, return e
off_t pop_nibble (uint *_U, uchar *B, off_t b, uint eob) { (void) eob;
  uint u = get_B_nibble (B, b);
  if (u<8) { *_U = u; return b+1; } // 0nnn
  uint U = 0, i=0, nibs = 1+(u&7);
  while (++i <= nibs) { u = get_B_nibble (B, ++b); U = (U<<4) | u; }
  *_U = U;  
  return b+1;
}

char *nibbl_encode (uint *U) {
  off_t b = 0, eob = 10*len(U); // max 9 nibbles per uint
  uchar *B = new_vec (eob/2, sizeof(char));
  uint *u = U-1, *end = U+len(U);
  while (++u < end) b = push_nibble (*u, B, b, eob);
  len(B) = (b>>1) + (b&1);
  return (char*)B;
}

uint *nibbl_decode (char *_B) {
  uchar *B = (uchar*)_B;
  off_t b = 0, end = len(B)*2;  
  uint *U = new_vec (0, sizeof(uint)), u=0;
  while (b < end) {
    b = pop_nibble (&u, B, b, end);
    if (u) U = append_vec (U, &u);
  }
  return U;
}

/*
off_t pop_nibble_maybe_fast (uint *U, uchar *B, off_t beg, off_t eob) {
  // mask for nibbles  1     12    1.3     1..4    1...5
  uint mask[9] = {0, 0xF0, 0xFF, 0xFFF0, 0xFFFF, 0xFFFFF0, 0xFFFFFF, 0xFFFFFFF0, 0xFFFFFFFF};
  uint byte = beg>>1;   // byte offset of this nibble
  uint left = !(beg&1); // is it left nibble? (0,2,4...)
  uint shift = left<<2; // left nibble -> bits 4..7 -> need to shift 4 right
  uchar nib = 0xF & (B[byte] >> shift); // shift left -> right, take bits 0..3
  if (nib < 8) { *U = nib; return beg+1; } // 0xxx -> self-contained nib -> return it
  uint U1=0, nibs = 1+(nib&7); // number of nibs we have to read
  if (left) { // consumed bits 4..7 but still have bits 0..3
    uchar nib0 = 0xF & B[byte]; // nib1 = bits 0..3
    --nibs; // one less nib to consume below
    U1 = nib0 << (nibs<<2); // make space for 4*nibs
  }
  uint *u = (uint*) (B+byte+1);
  uint U2 = (*u) & (mask[nibs]);
  *U = U1 | U2;
  return beg + nibs + 1;
}

off_t push_nibble_maybe_fast (uint U, uchar *B, off_t beg, off_t eob) {
  uint mask[9] = {0, 0xF0, 0xFF, 0xFFF0, 0xFFFF, 0xFFFFF0, 0xFFFFFF, 0xFFFFFFF0, 0xFFFFFFFF};
  uint byte = beg>>1, right = beg&1, left = !right, shift = left<<2;
  
  
  if (U<8) { B[byte] |= (U<<shift); return beg+1; }
  if (right)  { B[byte] |= (U & 0xF); U >>= 4; } // RHS of current byte = bits 
}
*/

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
  //len(B) = (b>>3) + ((b&7)>0); // fails on U[31116] -> B[34304] -> BU[31115]
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

// ---------------------------------------- bit-mask

char *bmask_encode (uint *U) {
  uint *u = U-1, *last = U+len(U)-1;
  char *B = bit_vec (*last);
  while (++u <= last) {
    bit_set_1(B,*u);
  }
  //printf ("bmask: U[%d] -> B[%d]\n", len(U), len(B));
  return B;
}

uint *bmask_decode (char *B) {
  //uchar *B = (uchar*)_B;
  uint *U = new_vec (0, sizeof(uint)), u=0, n = (len(B))<<3;
  for (u=0; u<n; ++u) {
    if (bit_is_1(B,u)) U = append_vec(U,&u);
  }
  return U;
}

// ---------------------------------------- z-standard

size_t ZSTD_compress( void* dst, size_t dstSz, const void* src, size_t srcSz, int lvl);
size_t ZSTD_decompress( void* dst, size_t dstSz, const void* src, size_t srcSz);
unsigned ZSTD_isError(size_t code);
const char* ZSTD_getErrorName(size_t code);

void zstd_assert (size_t sz, char *msg) {
  if (ZSTD_isError(sz)) {
    printf("%s ERROR: %s\n", msg, ZSTD_getErrorName(sz));
    assert(0);
  }
}

char *zstd_encode (uint *U) {
  size_t Usz = 4*len(U), Bsz = Usz+1024;
  void *B = new_vec (Bsz, 1);
  size_t sz = ZSTD_compress(B, Bsz, U, Usz, 5);
  zstd_assert (sz, "encode");
  len(B) = sz;
  return B;  
}

uint *zstd_decode (char *B) {
  size_t Bsz = len(B), Usz = 16*Bsz + (1<<28), esz = sizeof(uint);
  uint *U = new_vec (Usz/esz, esz);
  size_t sz = ZSTD_decompress(U, Usz, B, Bsz);
  zstd_assert (sz, "decode");
  len(U) = sz/esz;
  return U;
} 

// ---------------------------------------- pFORdelta

/*
int pfor_compress(unsigned int *input, unsigned int *output, int size);
int pfor_decompress(unsigned int* input, unsigned int* output, int size);

char *pford_encode (uint *U) {
  int SZ = 4*len(U);
  void *B = new_vec (SZ, 1);
  int sz = pfor_compress(U, (uint*)B, SZ);
  len(B) = sz;
  return B;  
}

uint *pford_decode (char *B) {
  int sz = len(B), EZ = 100*sz, esz = sizeof(uint);
  uint *U = new_vec (EZ/esz, esz);
  int SZ = pfor_decompress((uint*)B, U, EZ);
  len(U) = SZ/esz;
  return U;
} 
*/

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

uint *ix2i (ix_t *V, char x) {  
  uint i, n = len(V), *U = new_vec (n, sizeof(uint));
  if (x) for (i=0; i<n; ++i) U[i] = 1+(uint)V[i].x;
  else   for (i=0; i<n; ++i) U[i] = V[i].i;
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


void show_bufaz (uchar *B, uint a, uint z) {
  uchar *b = B+a-1, *end = B+z;
  while (++b < end) show_binary ((char)*b);  
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

int do_bmask (int n, char *A[]) {
  uint *V = argv2vec (A,n);
  show_vec (V, "original:");
  char *B = bmask_encode (V);
  show_buf (B, "encoded:");
  uint *D = bmask_decode (B);
  show_vec (D, "decoded:");
  free_vec(V); free_vec(B); free_vec(D);
  return 0;
}

int do_zstd (int n, char *A[]) {
  uint *V = argv2vec (A,n);
  show_vec (V, "original:");
  char *B = zstd_encode (V);
  show_buf (B, "encoded:");
  uint *D = zstd_decode (B);
  show_vec (D, "decoded:");
  free_vec(V); free_vec(B); free_vec(D);
  return 0;
}

int do_nibbl (int n, char *A[]) {
  uint *V = argv2vec (A,n);
  show_vec (V, "original:");
  char *B = nibbl_encode (V);
  show_buf (B, "encoded:");
  uint *D = nibbl_decode (B);
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

char *do_encode (uint *U, char a, char x) {
  if ((!x) && (a != 'b')) delta_encode(U);
  switch (a) {
  case 'v': return vbyte_encode(U);
  case 'm': return msint_encode(U);
  case 'n': return nibbl_encode(U);
  case 'g': return gamma_encode(U);
  case 'b': return bmask_encode(U);
  case 'z': return zstd_encode(U);
  default:  return NULL;
  }
}

uint *do_decode (char *B, char a, char x) {
  uint *U = NULL;
  switch (a) {
  case 'v': U = vbyte_decode(B); break;
  case 'm': U = msint_decode(B); break;
  case 'n': U = nibbl_decode(B); break;
  case 'g': U = gamma_decode(B); break;
  case 'b': U = bmask_decode(B); break;
  case 'z': U = zstd_decode(B); break;
  default:  break;
  }
  if ((!x) && (a != 'b')) delta_decode(U);
  return U;
}

int do_mtx (char *_M, char *alg) {
  coll_t *M = open_coll (_M, "r+");
  uint i, n = nvecs(M);
  double SZU = 0, SZB = 0, t0 = ftime();
  for (i = 1; i <= n; ++i) {
    ix_t *W = get_vec_ro(M,i);
    uint *U = ix2i (W,0), *V = copy_vec(U), j;
    char *B = do_encode (V, *alg, 0);
    uint *BU= do_decode (B, *alg, 0);
    double szU = vsize(U), szB = vsize(B), crB = 100*szB/szU;
    double CRB = 100*(SZB+=szB)/(SZU+=szU);
    double dT = ftime() - t0, MpS = (SZB/dT)/1E6;
    if (len(U) != len(BU)) {
      show_vec (U,  "orignal:");
      show_buf (B,  "encoded:");
      show_vec (BU, "decoded:");      
      assert (len(U) == len(BU));
    }
    for (j=0; j<len(U); ++j) 
      if (U[j] != BU[j]) return do_mtx_debug (i, U, BU, B, "B");
    if (i == next_pow2(i) || i == n)
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
  "compress -nibbl 1 2 3 4 5 6 7 8\n"
  "compress -gamma 1 2 3 4 5 6 7 8\n"
  "compress -bmask 1 2 3 4 5 6 7 8\n"
  "compress -zstd  1 2 3 4 5 6 7 8\n"
  "compress -mtx ROWSxCOLS alg\n"
  ;

#define arg(i) ((i < argc) ? argv[i] : NULL)
#define a(i) ((i < argc) ? argv[i] : "")

int main (int argc, char *argv[]) {
  if (argc < 2) return fprintf (stderr, "%s", usage);
  if (!strcmp (a(1),"-delta")) return do_delta(argc-2,argv+2);  
  if (!strcmp (a(1),"-vbyte")) return do_vbyte(argc-2,argv+2);
  if (!strcmp (a(1),"-msint")) return do_msint(argc-2,argv+2);
  if (!strcmp (a(1),"-nibbl")) return do_nibbl(argc-2,argv+2);
  if (!strcmp (a(1),"-gamma")) return do_gamma(argc-2,argv+2);
  if (!strcmp (a(1),"-bmask")) return do_bmask(argc-2,argv+2);
  if (!strcmp (a(1),"-zstd")) return do_zstd(argc-2,argv+2);
  if (!strcmp (a(1),"-mtx")) return do_mtx(arg(2), a(3));
  return 1;
}

#endif
