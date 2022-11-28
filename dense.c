#include "matrix.h"
uint is_dense (ix_t *V);

void show_slice (float *S, uint nr, uint nc, const char *tag) {
  if (tag) printf("%s [%d x %d]\n", tag, nr, nc);
  for (uint r = 0; r < nr; ++r) {
    for (uint c = 0; c < nc; ++c) printf (" %6.2f", S[r*nc+c]);
    putchar('\n');
  }
}

// insert row-major float[] into M [r0:r0+nr, c0:c0+nc]
void slice_to_mtx (coll_t *M, uint r0, uint c0, uint nr, uint nc, float *S) {
  for (uint r = 0; r < nr; ++r) {
    uint id = r0+r+1;
    ix_t *row = has_vec (M,id) ? get_vec (M,id) : new_vec (0, sizeof(ix_t));
    //fprintf(stderr, "resizing [%d] row: %d -> %d\n", (r0+r+1), len(row), (c0+nc));
    row = resize_vec (row, c0+nc); // make sure row has space for columns c0...c0+nc
    for (uint c = 0; c < nc; ++c) {
      row[c0+c].i = c0+c+1;
      row[c0+c].x = S[r*nc + c];
    }
    put_vec (M, r0+r+1, row);
    free_vec (row);
  }
}

// M [r0:r0+nr, c0:c0+nc] -> S: pre-allocated to [nr * nc]  floats
void mtx_to_slice (coll_t *M, uint r0, uint c0, uint nr, uint nc, float *S) {
  for (uint r = 0; r < nr; ++r) {
    ix_t *row = get_vec_ro (M,r0+r+1);
    assert (is_dense(row));
    for (uint c = 0; c < nc; ++c) S[r*nc + c] = row[c0+c].x;
  }
}
