
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
#include "matrix.h"

#ifndef CLUSTER
#define CLUSTER

//ix_t **doc_vecs(ix_t *D, coll_t *DOCS) ; // deprecated
float **pairwise_sims (coll_t *A, coll_t *B) ;
float **self_sims (coll_t *D) ;
void show_sims (float **sim) ;

// ------------------------- TDT-like -------------------------

void one_per_clump(jix_t *C) ;
jix_t *clump_docs (ix_t *D, coll_t *DOCS, char *prm) ;

// ------------------------- MST -------------------------

jix_t *docs_mst (ix_t *docs, float **sim) ;
void mst_dot (jix_t *mst, hash_t *ID) ; //digraph { a -> b[label="0.2"]; }

// ------------------------- k-means -------------------------

void k_means (coll_t *DxW, uint K, int iter, coll_t **_KxD, coll_t **_KxW) ;
float *inverse_cluster_frequency (coll_t *KxW) ;
void cluster_signatures (coll_t *KxW) ;

coll_t *subset_rows (coll_t *M, ix_t *rows) ;
uint *column_map (coll_t *R) ;
uint *invert_map (uint *map) ;
void renumber_cols (coll_t *R, uint *map) ;
coll_t *transpose_inmem (coll_t *ROWS) ;


#endif
