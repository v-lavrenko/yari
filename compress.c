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


#define last_byte(char b) b & 

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
    *end++ = U & 127; // 7 lowest bits (vbyte) -> buffer
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
    *U <<= 7; // make space for 7 bits
    *U |= (*buf & 127); // vbyte -> 7 lowest bits
    if (*buf++ & 128) break; // vbyte null-terminated
  }
  return buf; // new end of buffer
}

char *vbyte_encode (uint *U, uint n, char *buf) {
  uint i;
  for (i = 0; i < n; ++i) buf = push_vbyte (U[i], buf);
  return buf;
}

char *vbyte_decode () {
  
}

// ---------------------------------------- delta encoding

void delta_encode (uint *V) {
  uint *v = V + len(V);
  while (--v > V) v->x -= (v-1)->x;
}

void delta_decode (uint *V) {
  uint *v = V, *end = V + len(V);
  while (++v < end) v->x += (v-1)->x;
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
