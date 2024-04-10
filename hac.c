/*

  Copyright (c) 1997-2024 Victor Lavrenko (v.lavrenko@gmail.com)

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
#include "hac.h"

inline uint tri (uint i, uint j) { // dense lower-triangular matrix
  if (i >= j) return i*(i-1)/2 + j;
  else        return j*(j-1)/2 + i;
}

inline uint trilen (uint n) { return (ceil(sqrt(1+8*n)) - 1) / 2; } // n = i(i+1)/2

// sparse symmetric matrix -> dense lower-triangular form
float *mtx2tri (coll_t *M) {
  uint nr = num_rows(M), nc = num_cols(M);
  if (nr != nc) fprintf (stderr, "%s [%d x %d] should be square\n", M->path, nr, nc);
  float *T = 0;

}

// Lance-Williams algorithm
// i,j = argmin D(i,j) ... find pair of closest clusters
// new cluster n = i+j
// for each remaining cluster k != i,j:
//     D(k,n) = a D(k,i) + b D(k,j) + c D(i,j) + d |D(k,i) - D(k,j)|
// delete clusters i,j
uint hac(float *D) {


}
