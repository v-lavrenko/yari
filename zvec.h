/*

  Copyright (c) 1997-2025 Victor Lavrenko (v.lavrenko@gmail.com)

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

// any buffer <-> zstd-vector.
byte *buf_to_zvec (void *buf, size_t buf_sz, int level) ;
void *zvec_to_buf (byte *zvec, size_t *result_sz) ;

// C string <-> zstd-vector.
byte *str_to_zvec(char *s, int level) ;
char *zvec_to_str (byte *zvec) ;

// any vector -> zstd-vector.
byte *vec_to_zvec (void *vec, int level) ;
void *zvec_to_vec (byte *zvec, uint el_size) ;

// compressed put/get of a regular vector
void put_vec_zst (coll_t *c, uint id, void *vec) ;
void *get_vec_zst (coll_t *c, uint id, uint el_size) ;

// compress put/get chunk
void put_chunk_zst (coll_t *c, uint id, void *chunk, size_t sz) ;
void *get_chunk_zst (coll_t *c, uint id, size_t *sz) ;
void *get_string_zst (coll_t *c, uint id) ;

// convert string to vector of bytes (resizeable string with length).
byte *str_to_bytes(char *str) ;

// for compatibility with compress.{c,h}
byte *zstd_encodeU (uint *U) ; // zstdlib
uint *zstd_decodeU (byte *B) ;
