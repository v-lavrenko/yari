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

// simple randomized helpers
#include "vector.h"

#define loginc(n) (n + ((random() / 2147483647) < (1/n)))

#define loginc(n) (n + (log(n) + log(random()) <  log(2147483647)))

#define getbit(B,i) (B[i>>5] &   (1 << (i & 31)))
#define setbit(B,i) (B[i>>5] |=  (1 << (i & 31)))
#define clrbit(B,i) (B[i>>5] &= ~(1 << (i & 31)))

uint *bitvec(uint N) { return new_vec (N/32, sizeof(uint)); }


void bloom_add (uint *B, char *el) {
  
}

void bloom_has (uint *B, char *el) {
  
}
