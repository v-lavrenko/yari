#ifndef BITVEC
#define BITVEC

// char-level
#define bit_vec(N) (new_vec(1+((N)>>3), sizeof(char)))
#define bit_is_1(B,i)  (B[i>>3] &   (128 >> (i & 7)))
#define bit_set_1(B,i) (B[i>>3] |=  (128 >> (i & 7)))
#define bit_set_0(B,i) (B[i>>3] &= ~(128 >> (i & 7)))

#endif
