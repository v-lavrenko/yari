
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
