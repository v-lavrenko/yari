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

#ifndef SPELLING
#define SPELLING

// delete/transpose/insert/substitute 'a' into string at X[0..n) at position i
int do_edit (char *X, int n, int i, char op, char a) ;

// insert all edits of word into set
int all_edits (char *word, uint nedits, hash_t *edits) ;

// *results = all edits that match the known hash, with scores
int known_edits (char *word, uint nedits, hash_t *known, float *score, ix_t **result) ;

// *best = id of edit that has highest score
int best_edit (char *word, uint nedits, hash_t *known, float *score, uint *best) ;

// aminoacid -> {i:amino, j:acid, x: MIN(F[amino],F[acid])}
void try_runon (char *word, hash_t *known, float *score, uint *i, uint *j) ;

// amino acid -> aminoacid
uint try_fuse (char *w1, char *w2, hash_t *known) ;

uint pubmed_spell (char *word, hash_t *H, float *F, char *prm, uint W, uint *id2) ;

uint levenstein_distance (char *A, char *B, char *explain) ;

#endif
