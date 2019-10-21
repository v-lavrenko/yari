// simple randomized helpers
#include "vector.h"

#define loginc(n) (n + ((random() / 2147483647) < (1/n)))

#define loginc(n) (n + (log(n) + log(random()) <  log(2147483647)))

#define getbit(B,i) (B[i>>5] &   (1 << (i & 31)))
#define setbit(B,i) (B[i>>5] |=  (1 << (i & 31)))
#define clrbit(B,i) (B[i>>5] &= ~(1 << (i & 31)))

uint *bitvec(uint N) { return new_vec (N/32, sizeof(uint)); }

