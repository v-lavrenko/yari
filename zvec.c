#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "hash.h"
#include "timeutil.h"

// ---------------------------------------- z-standard

size_t ZSTD_compress( void* dst, size_t dstSz, const void* src, size_t srcSz, int lvl);
size_t ZSTD_decompress( void* dst, size_t dstSz, const void* src, size_t srcSz);
unsigned ZSTD_isError(size_t code);
const char* ZSTD_getErrorName(size_t code);
size_t ZSTD_compressBound(size_t srcSize);
unsigned long long ZSTD_getDecompressedSize(const void* src, size_t srcSize);
unsigned long long ZSTD_getFrameContentSize(const void *src, size_t srcSize);

// if sz == error, print msg followed by zstd error.
void zstd_assert (size_t sz, char *msg) {
  if (ZSTD_isError(sz)) {
    fprintf(stderr, "%s ZSTD ERROR: [%ld] %s\n", msg, sz, ZSTD_getErrorName(sz));
    assert(0 && "YARI: compress.c:zstd_assert()");
  }
}

// ---------- work on uint *U, like delta/vbyte/gamma ----------

// compress: vector of ints U -> vector of bytes B.
// crudely pre-allocate B
char *zstd_encodeU (uint *U) {
  size_t Usz = 4*len(U), Bsz = Usz+1024;
  void *B = new_vec (Bsz, 1);
  size_t sz = ZSTD_compress(B, Bsz, U, Usz, 5);
  zstd_assert (sz, "encode");
  len(B) = sz;
  return B;
}

// decompress: vector of chars B -> vector of unts U.
// crudely preallocate U
uint *zstd_decodeU (char *B) {
  size_t Bsz = len(B), Usz = 16*Bsz + (1<<28), esz = sizeof(uint);
  uint *U = new_vec (Usz/esz, esz);
  size_t sz = ZSTD_decompress(U, Usz, B, Bsz);
  zstd_assert (sz, "decode");
  len(U) = sz/esz;
  return U;
}

// ---------- work on chunks of bytes ----------

// compress chunk src[:ssz] into re-allocated trg[:sz]
void UNUSED_zstd (char **trg, size_t *sz, char *src, size_t ssz, int level) {
  size_t SZ = ZSTD_compressBound(ssz); // worst-case compressed size
  *trg = safe_realloc(*trg, SZ);
  *sz = ZSTD_compress(*trg, SZ, src, ssz, level);
  zstd_assert(*sz, "encode");
}

// uncompress src[:ssz] into re-allocated trg[:sz]
void UNUSED_unzstd (char **trg, size_t *sz, char *src, size_t ssz) {
  size_t SZ = ZSTD_getFrameContentSize(src, ssz);
  *trg = safe_realloc(*trg, SZ);
  *sz = ZSTD_decompress(*trg, SZ, src, ssz);
  zstd_assert(*sz, "decode");
}

// compress src[:ssz] and write it into c[id].
// BUG: chunk_sz(id) != number of compressed bytes.
void BUGGY_put_chunk_zstd (coll_t *c, uint id, char *src, size_t ssz) {
  char *buf=NULL; size_t used=0;
  UNUSED_zstd(&buf, &used, src, ssz, 3);
  put_chunk_pwrite(c, id, buf, used); // <-- BUG
  free(buf);
}

// return a decompressed copy of chunk c[id].
// BUG: assumes chunk_sz == compressed bytes.
char *BUGGY_get_chunk_zstd (coll_t *c, uint id) {
  if (!has_vec(c, id)) return NULL;
  char *buf=NULL; size_t used=0;
  char *src = get_chunk(c, id);
  size_t ssz = chunk_sz(c, id); // <-- BUG
  UNUSED_unzstd(&buf, &used, src, ssz);
  return buf;
}

// -------------- any vector <-> compressed vector ---------------

// convert string to vector of bytes (resizeable string with length).
byte *str_to_bytes(char *str) {
  uint sz = strlen(str) + 1; // include terminating \0
  byte *vec = new_vec(0, sizeof(byte));
  return append_many(vec, str, sz);
}

// compress any buffer -> zstd-vector.
byte *buf_to_zvec (void *buf, size_t buf_sz, int level) {
  size_t max_sz = ZSTD_compressBound(buf_sz); // worst-case compressed size
  byte *zvec = new_vec(max_sz, sizeof(byte));
  size_t zvec_sz = ZSTD_compress(zvec, max_sz, buf, buf_sz, level);
  zstd_assert(zvec_sz, "compress");
  len(zvec) = zvec_sz;
  return zvec;
}

// uncompress zstd-vector -> buffer (return size in *result_sz).
void *zvec_to_buf (byte *zvec, size_t *result_sz) {
  size_t max_sz = ZSTD_getFrameContentSize(zvec, len(zvec));
  void *buf = safe_malloc (max_sz);
  size_t buf_sz = ZSTD_decompress(buf, max_sz, zvec, len(zvec));
  zstd_assert(buf_sz, "decompress");
  //if (buf_sz < max_sz) buf = safe_realloc(buf, buf_sz);
  if (result_sz) *result_sz = buf_sz;
  return buf;
}

// compress string -> zstd-vector.
byte *str_to_zvec(char *s, int level) {
  return buf_to_zvec (s, strlen(s)+1, level);
}

// uncompress zstd-vector -> string.
char *zvec_to_str (byte *zvec) {
  size_t sz; char *buf = zvec_to_buf (zvec, &sz);
  if (sz > 0) buf[sz-1] = '\0'; // null-terminate
  return buf;
}

// compress any vector -> zstd-vector.
byte *vec_to_zvec (void *vec, int level) {
  return buf_to_zvec (vec, len(vec) * vesize(vec), level);
}

// uncompress zstd-vector -> vector of given el_size.
void *zvec_to_vec (byte *zvec, uint el_size) {
  size_t max_sz = ZSTD_getFrameContentSize(zvec, len(zvec));
  uint max_els = ((max_sz - 1) / el_size) + 1;
  void *vec = new_vec(max_els, el_size);
  size_t vec_sz = ZSTD_decompress(vec, max_sz, zvec, len(zvec));
  zstd_assert(vec_sz, "decompress");
  len(vec) = ((vec_sz - 1) / el_size) + 1;
  return vec;
}

// compress vec and write to c[id]
void put_vec_zst (coll_t *c, uint id, void *vec) {
  byte *zvec = vec_to_zvec (vec, 5);
  put_vec_write (c, id, zvec);
  free_vec(zvec);
}

// return uncompressed copy of c[id]
void *get_vec_zst (coll_t *c, uint id, uint el_size) {
  byte *zvec = get_vec_ro (c, id);
  if (!len(zvec)) return new_vec(0,0);
  return zvec_to_vec(zvec, el_size);
}

// compress chunk and write to c[id]
void put_chunk_zst (coll_t *c, uint id, void *chunk, size_t sz) {
  byte *zvec = buf_to_zvec(chunk, sz, 5);
  put_vec_write (c, id, zvec);
  free_vec(zvec);
}

// return uncompressed copy of c[id], return size in *sz.
void *get_chunk_zst (coll_t *c, uint id, size_t *sz) {
  byte *zvec = get_vec_ro (c, id);
  if (!len(zvec)) return NULL;
  return zvec_to_buf(zvec, sz);
}

// return uncompressed copy of string c[id].
void *get_string_zst (coll_t *c, uint id) {
  byte *zvec = get_vec_ro (c, id);
  if (!len(zvec)) return NULL;
  return zvec_to_str(zvec);
}
