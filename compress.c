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

void reverse (char *A, char *Z) {
  for (--Z; A < Z; ++A, --Z) {
    char tmp = *A; *A = *Z; *Z = tmp;
  }
}

char *uint2vbyte (uint U, char *V) {
  char *_V = V;
  do {
    *V++ = U & 127; // 7 lowest bits -> vbyte
    U >>= 7; // shift out 7 lowest bits
  } while (U); // nothing to encode
  V[-1] |= 128; // terminate last vbyte
  reverse (_V, V);
  return V;
}

uint vbyte2uint (char *V) {
  uint U = 0;
  do {
    U <<= 7;
    U |= (*V & 127); // vbyte -> 7 lowest bits
    if (*V++ & 128) break;
  }
  return U;
}


char *uint2vbyte (uint *U, uint n) {
  uint i;
  for (i = 0; i < n; ++i) {
    uint u = i ? U[i] - U[i-1] : 
    
  }
}


void delta_encode (uint *V) {
  uint *v = V + len(V);
  while (--v > V) v->x -= (v-1)->x;
}

void delta_decode (uint *V) {
  uint *v = V, *end = V + len(V);
  while (++v < end) v->x += (v-1)->x;
}

int is_constant (float *X) {
  float *x = X, *end = X + len(X);
  while (++x < end) if (*x != *(x-1)) return 0;
  return 1;
}

ix_t *count_values (float *X) {
  uint n = len(X);
  ix_t *V = new_vec (n, sizeof(ix_t)), *v, *u, *end = V+n;
  for (v = V; v < end; ++v) { v->x = X[v-V]; v->i = 1; }
  sort_vec (V, cmp_ix_x);
  for (u = v = V; v < end; ++v) {
    if (u == v) continue;
    if (u->x == v->x) u->i += 1;
    else if (++u < v) *u = *v;
  }
  len(V) = u - V + 1;
  sort_vec (V, cmp_ix_I);
  return V;
}
