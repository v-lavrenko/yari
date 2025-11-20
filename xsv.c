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

#include "coll.h"
#include "hash.h"
#include "textutil.h"

// ---------------------------------------------------------------- header -> dict

// read 1st line from stdin, split into keys, insert into DICT
int xsv_header (char *DICT) {
  hash_t *D = open_hash (DICT, "w");
  char *buf = 0;
  size_t sz=0, nb = getline (&buf,&sz,stdin); // read 1st line
  if (buf[nb-1] == '\n') buf[--nb] = '\0'; // strip \n
  if (buf[nb-1] == '\r') buf[--nb] = '\0'; // strip \r
  char **K = split (buf, '\t'), **k;
  for (k = K; k < K+len(K); ++k) key2id (D,*k);
  fprintf (stderr, "%s: %d keys\n", DICT, nkeys(D));
  free_vec(K); free (buf); free_hash(D);
  return 0;
}

// ---------------------------------------------------------------- cells -> KVS

int xsv_load (char *cells, char *rows) {
  coll_t *C = open_coll (cells, "w");
  coll_t *R = open_coll (rows, "w+");
  size_t sz = 0, nb = 0, nr = 0, nc = 0;
  char *row = 0;
  getline(&row,&sz,stdin); // skip 1st line (header)
  while (0 < (nb = getline(&row,&sz,stdin))) { // read a line
    if (row[nb-1] == '\n') row[--nb] = '\0'; // strip \n
    if (row[nb-1] == '\r') row[--nb] = '\0'; // strip \r

    it_t *ids = new_vec(0,sizeof(it_t));
    char **V = split(row,'\t'); // values in the row
    uint i, n = len(V);
    for (i=0; i<n; ++i) { // for each value
      off_t sz = (i+1<n) ? (V[i+1]-V[i]) : (row+nb-V[i]);
      if (!sz) continue;
      put_chunk (C, ++nc, V[i], sz); // value string -> C
      it_t new = {i+1, nc};
      ids = append_vec (ids, &new); // value id -> ids
    }
    put_vec (R, ++nr, ids);
    free_vec (ids);
    free_vec (V);
    if (0 == nr%1000) show_progress (nr>>20, 0, "M rows");
  }
  free_coll (C); free_coll(R); free (row);
  return 0;
}

// ---------------------------------------------------------------- CSV offsets

/*
off_t xsv_push_header (char *tsv, char *dict) {
  if (!dict) return 0; // skip header
  hash_t *H = open_hash (dict, "w"); uint i;
  off_t sz = strcspn (tsv, "\r\n"); // row size
  char *row = strndup (tsv, sz);
  char **keys = split (row, '\t'); // split row into columns
  for (i = 0; i < len(keys); ++i)
    key2id (H, keys[i]);  // insert column names into dict
  fprintf (stderr, "%s: %d keys\n", hdr, nkeys(H));
  free_hash (H);
  free_vec (keys);
  free (row);
  return sz + strspn (tsv+sz, "\r\n"); // next row
}
*/

// return index of c in buf [beg:end), or end if not found
static inline off_t find (char c, char *buf, off_t beg, off_t end) {
  while (beg < end && buf[beg] != c) ++beg;
  return (beg < end) ? beg : end;
}

int xsv_index_rows (char *_tsv, char *rows) {
  mmap_t *M = open_mmap (_tsv, "r", file_size(_tsv)); // mmap whole tsv
  char *buf = M->data;
  coll_t *R = open_coll (rows, "w+"); // cell offsets for each row
  fprintf (stderr, "%s %d x %d\n", R->path, R->rdim, R->cdim);
  off_t *row = new_vec (0, sizeof(off_t)); // offsets for one row
  off_t o = 0, flen = M->flen, NR = 0, NC = 0;
  while (o < flen) {
    off_t eol = find('\n', buf, o, flen);
    while (o < eol) { // for every cell in row
      row = append_vec (row, &o); // start of this cell
      o = find('\t', buf, o+1, eol); // end of this cell
    }
    assert (o == eol);
    row = append_vec (row, &o); // offset where row ends
    put_vec (R, ++NR, row);
    R->rdim = NR;
    R->cdim = NC = MAX(NC,len(row));
    len(row) = 0;
    o = eol+1; // next line starts after newline
    if (0 == NR%1000) show_progress (NR>>20, 0, "M rows");
  }
  printf("%s [%d x %d] %ldMB\n", _tsv, R->rdim, R->cdim, (ulong)(o>>20));
  free_vec (row); free_coll (R); free_mmap (M);
  return 0;
}

int xsv_do_get (char *tsv, char *_R, char *row, char *col) {
  mmap_t *M = open_mmap (tsv, "r", file_size(tsv));
  coll_t *R = open_coll (_R, "r+");
  uint r = atoi(row), c = atoi(col);
  off_t *offs = get_vec_ro(R,r);
  off_t beg = offs[c-1], end = offs[c];
  char *val = strndup(M->data+beg, end-beg);
  printf ("%s [%d,%d] = [%ld..%ld] = '%s'\n", tsv, r, c, (ulong)beg, (ulong)end, val);
  free_mmap (M); free_coll(R); free(val);
  return 0;
}

int xsv_size (char *_R) {
  coll_t *R = open_coll (_R, "r+");
  printf("%s: %u x %u\n", _R, R->rdim, R->cdim);
  free_coll(R);
  return 0;
}

// ---------------------------------------------------------------- MAIN

//#ifdef MAIN

#define arg(i) ((i < argc) ? argv[i] : NULL)
#define a(i) ((i < argc) ? argv[i] : "")

char *usage =
  "xsv dict H < tsv    ... column names -> dictionary H\n"
  "xsv load C R < tsv ... load tsv from stdin\n"
  "                        C: all non-empty cell values\n"
  "                        R: row -> list of columns\n"
  "xsv rows tsv R     ... R[r,c] = offset of [r,c] in tsv\n"
  "xsv size R         ... show number of rows and columns\n"
  "xsv get tsv R r c  ... get tsv[r,c]\n"
  ;

int main (int argc, char *argv[]) {
  if (!strcmp(a(1),"dict"))  return xsv_header (arg(2));
  if (!strcmp(a(1),"load")) return xsv_load (arg(2), arg(3));
  if (!strcmp(a(1),"rows")) return xsv_index_rows (arg(2), arg(3));
  if (!strcmp(a(1),"size")) return xsv_size (arg(2));
  if (!strcmp(a(1),"get"))  return xsv_do_get (arg(2), arg(3), a(4), a(5));
  fprintf(stderr,"%s",usage);
  return 0;
}

//#endif
