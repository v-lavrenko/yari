/*
  
  Copyright (c) 1997-2016 Victor Lavrenko (v.lavrenko@gmail.com)
  
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

#include <utime.h>
#include "coll.h"

off_t MIN_OFFS = 8;

int coll_exists (char *path) { return file_exists ("%s/coll.vecs", path); }

time_t coll_modified (char *path) { return file_modified ("%s/coll.vecs",path); }

static inline coll_t *open_coll_inmem () {
  coll_t *c = safe_calloc (sizeof(coll_t)); 
  c->offs = new_vec (1, sizeof(off_t)); 
  return c; 
}

coll_t *copy_coll (coll_t *src) { 
  coll_t *trg = open_coll_inmem ();
  uint i = 0, n = len (src->offs) - 1;
  while (++i <= n) put_vec (trg, i, get_vec_ro (src,i));
  return trg;
}

coll_t *open_coll (char *path, char *access) {
  if (!path) return open_coll_inmem ();
  coll_t *c = safe_calloc (sizeof(coll_t));
  if (*access != 'r') { mkdir (path, S_IRWXU | S_IRWXG | S_IRWXO); utime (path, NULL); }
  c->access = strdup (access);
  c->path = path = strdup (path);
  char x[9999];
  FILE *info = fopen(fmt(x,"%s/coll.info",path), "r");
  if (!info || !fscanf(info,"vers: %d\n", &(c->version))) c->version = COLL_VERSION;
  if (!info || !fscanf(info,"rows: %d\n", &(c->rdim)))    c->rdim = 0;
  if (!info || !fscanf(info,"cols: %d\n", &(c->cdim)))    c->cdim = 0;
  if (info) fclose(info);
  if (c->version != COLL_VERSION) { fprintf (stderr, "ERROR: version of %s: %d != %d\n", path, c->version, COLL_VERSION); exit(1); }
  c->vecs = open_mmap (fmt(x,"%s/coll.vecs",path), access, MAP_SIZE);
  c->offs = open_vec (fmt(x,"%s/coll.offs",path), access, sizeof(off_t));
  if (access[1] == '+') {
    c->prev = open_vec (fmt(x,"%s/coll.prev",path), access, sizeof(uint));
    c->next = open_vec (fmt(x,"%s/coll.next",path), access, sizeof(uint));
  } else c->prev = c->next = NULL;
  if (len(c->offs) == 0) { // new matrix => initialize it
    c->offs = resize_vec (c->offs,1); 
    c->prev = resize_vec (c->prev,1); // NULL if c->prev was NULL
    c->next = resize_vec (c->next,1); 
    c->offs[0] = MIN_OFFS;
  }
  return c;
}

static inline void free_coll_inmem (coll_t *c) {
  off_t *o = c->offs - 1, *end = c->offs + len(c->offs);
  while (++o < end) free (NULL + *o);
  free_vec (c->offs);
  memset (c, 0, sizeof(coll_t));
  free (c);
}

void free_coll (coll_t *c) {
  if (!c) return;
  if (!c->path) return free_coll_inmem (c);
  free_mmap (c->vecs);
  free_vec (c->offs);
  free_vec (c->prev);
  free_vec (c->next);
  if (c->access[0] != 'r') {
    char x[9999];
    FILE *info = safe_fopen(fmt(x,"%s/coll.info",c->path), "w");
    fprintf(info,"vers: %d\n", COLL_VERSION);
    fprintf(info,"rows: %d\n", c->rdim);
    fprintf(info,"cols: %d\n", c->cdim);
    fclose(info);
    utime (c->path, NULL); // touch path
  }
  free (c->path);
  free (c->access);
  free (c);
}

coll_t *reopen_coll (coll_t *c, char *access) {
  char *path = c->path ? strdup(c->path) : NULL;
  free_coll (c);
  c = open_coll (path, access);
  if (path) free (path);
  return c;
}

void rm_coll (coll_t *c) {
  char *dir = c->path ? strdup (c->path) : NULL;
  free_coll (c);
  if (dir) rm_dir (dir);
  if (dir) free (dir);
}

void empty_coll (coll_t *c) {
  uint i = 0, n = nvecs(c);
  while (++i <= n) del_vec (c,i);
}

static inline void *get_chunk_inmem (coll_t *c, uint id) {
  return (id < len(c->offs)) ? (NULL + c->offs[id]) : NULL; 
}

static inline void del_chunk_inmem (coll_t *c, uint id) {
  if (id >= len(c->offs)) return;
  free (NULL + c->offs[id]);
  c->offs[id] = 0; 
}

static inline void *map_chunk_inmem (coll_t *c, uint id, off_t size) {
  if (id >= len(c->offs)) c->offs = resize_vec (c->offs, id+1);
  void *trg = safe_realloc (NULL + c->offs[id], size);
  c->offs[id] = trg - NULL; 
  return trg;
}

static inline void put_chunk_inmem (coll_t *c, uint id, void *buf, off_t size) {
  if (id >= len(c->offs)) c->offs = resize_vec (c->offs, id+1);
  void *trg = safe_realloc (NULL + c->offs[id], size);
  memcpy (trg, buf, size);
  c->offs[id] = trg - NULL; 
}

off_t chunk_sz (coll_t *c, uint id) {
  assert (c->path && has_vec (c,id));
  uint next = c->next ? c->next[id] : (id+1) % len(c->offs);
  return c->offs[next] - c->offs[id];
}

// unlink id from its chunk, allocate chunk to preceding id
// use only on collections opened as 'a+' or 'w+'
void del_chunk (coll_t *c, uint id) {
  if (!c->path) return del_chunk_inmem (c,id);
  if (!has_vec(c,id)) return;
  if (c->next) {
    c->offs[id] = 0; // mark id as non-existent (can't do this if no next)
    uint next = c->next[id], prev = c->prev[id]; // prev <-> id <-> next
    c->next [prev] = next;                       // prev ---------> next
    c->prev [next] = prev;                       // prev <--------- next
    c->prev [id] = c->next [id] = 0;             // disconnect id
  }
}

static inline void mov_chunk (coll_t *c, uint id, off_t sz) { // NEVER call directly
  if (id+1 >= len(c->offs)) { // haven't seen this id before
    c->offs = resize_vec (c->offs, id+1);
    c->prev = resize_vec (c->prev, id+1);
    c->next = resize_vec (c->next, id+1);
  }
  uint next = c->next ? c->next[id] : (id+1) % len(c->offs); // next id
  off_t offs = c->offs [id], stop = c->offs [next];
  if (offs && offs+sz <= stop) return; // exists and fits in current slot
  if (offs) del_chunk (c, id); // doesn't fit => delete it, add at the end
  if (c->next) {
    uint head = 0, tail = c->prev[head]; // tail <------> head
    c->next[id] = head;                  //         id -> head
    c->prev[head] = id;                  //         id <- head
    c->prev[id] = tail;                  // tail <- id
    c->next[tail] = id;                  // tail -> id
  }
  c->offs[id] = c->offs[0];              // old end of heap => our offset
  c->offs[0] += align8(sz);              // new end of heap 
  grow_mmap_file (c->vecs, c->offs[0]);  // make sure file is big enough
}

// return pointer to chunk linked by id, or NULL
void *get_chunk (coll_t *c, uint id) {
  if (!c->path) return get_chunk_inmem(c,id);
  if (!has_vec(c,id)) return NULL;
  uint next = c->next ? c->next[id] : (id+1) % len(c->offs);
  off_t size = c->offs[next] - c->offs[id];
  return move_mmap (c->vecs, c->offs[id], size); 
}

// return a copy of chunk linked by id, or NULL => must be freed
void *get_chunk_pread (coll_t *c, uint id) {
  if (!c->path) return get_chunk_inmem(c,id);
  if (!has_vec(c,id)) return NULL;
  uint next = c->next ? c->next[id] : (id+1) % len(c->offs);
  off_t offs = c->offs[id], size = c->offs[next] - offs;
  char *trg = safe_malloc (size);
  mmap_t *M = c->vecs;
  if (offs >= M->offs && (offs+size) <= (M->offs + M->size)) { 
    char *src = M->data + (offs - M->offs); // inside mmap
    memcpy (trg, src, size);
  } else safe_pread (M->file, trg, size, offs); // outside
  return trg;
}

static inline void *map_chunk (coll_t *c, uint id, off_t size) {
  if (!c->path) return map_chunk_inmem (c,id,size);
  mov_chunk (c, id, size);
  return move_mmap (c->vecs, c->offs[id], size);
}

void put_chunk (coll_t *c, uint id, void *src, off_t size) {
  if (!c->path) return put_chunk_inmem (c,id,src,size);
  mov_chunk (c, id, size);
  void *trg = move_mmap (c->vecs, c->offs[id], size); 
  memcpy (trg, src, size);
}

void put_chunk_pwrite (coll_t *c, uint id, void *chunk, off_t size) {
  if (!c->path) return put_chunk_inmem (c,id,chunk,size);
  mov_chunk (c, id, size);
  safe_pwrite (c->vecs->file, chunk, size, c->offs[id]);
}

vec_t nullvec = {0, 0, 0, 0, {}};

void *get_vec (coll_t *c, uint id) {
  vec_t *hdr = get_chunk (c, id);
  return hdr ? copy_vec (hdr->data) : new_vec (0, 0);
}

void *get_vec_read (coll_t *c, uint id) {
  vec_t *hdr = get_chunk_pread (c, id);
  if (!hdr) return new_vec (0,0);
  hdr->file = 0; // TODO: FIX THIS!
  return hdr->data;
}

/* redundant: new get_vec() will always copy */
void *get_vec_ro (coll_t *c, uint id) {
  vec_t *hdr = get_chunk (c, id);
  return hdr ? hdr->data : (&nullvec)->data;
}
/**/

/* redundant: new get_vec() will always pread */
void *get_vec_mp (coll_t *c, uint id) { // thread-safe
  if (!has_vec(c,id)) return new_vec (0, sizeof(ix_t));
  if (!c->path) { // in-memory
    vec_t *vec = NULL + c->offs[id];
    return copy_vec (vec->data); 
  } 
  mmap_t *M = c->vecs;
  uint next = c->next ? c->next[id] : (id+1) % len(c->offs);
  off_t offs = c->offs[id], size = c->offs[next] - offs;
  if (offs >= M->offs && (offs+size) <= (M->offs + M->size)) { // in mmap
    vec_t *vec = (vec_t*) (M->data + (offs - M->offs)); 
    return copy_vec (vec->data); 
  } 
  vec_t *vec = malloc(size);
  safe_pread (M->file, vec, size, offs); 
  vec->file = 0; // vector in memory
  return vec->data;
}
/**/

/**/
void *get_or_new_vec (coll_t *c, uint id, uint esize) {
  vec_t *hdr = get_chunk (c, id);
  return hdr ? copy_vec (hdr->data) : new_vec (0, esize);
}
/**/

void del_vec (coll_t *c, uint id) { del_chunk (c, id); }

static inline void update_dims (coll_t *c, uint id, ix_t *V, uint n, uint sz) {
  if (id > c->rdim) c->rdim = id;
  if (sz == sizeof(ix_t) && n && V[n-1].i > c->cdim) c->cdim = V[n-1].i;
}

/**/
void put_vec (coll_t *c, uint id, void *vec) {
  if (!vec || !len(vec)) return del_vec (c,id);
  vec_t *src = vect(vec);
  update_dims (c, id, vec, src->count, src->esize);
  off_t size = sizeof(vec_t) + src->count * src->esize;
  if (!c->path) return put_chunk_inmem (c,id,src,size);
  vec_t *trg = map_chunk (c, id, size);
  memcpy (trg, src, size);
  trg->file = 2; // should be 2 for vectors in a collection
}
/**/

void put_vec_write (coll_t *c, uint id, void *vec) {
  if (!vec || !len(vec)) return del_vec (c,id);
  vec_t *src = vect(vec);
  off_t size = sizeof(vec_t) + src->count * src->esize;
  uint file = src->file; // TODO: FIX THIS
  src->file = 2; // should be 2 for vectors in a collection
  put_chunk_pwrite (c, id, src, size);
  update_dims (c, id, vec, src->count, src->esize);
  src->file = file;
}

void *map_vec (coll_t *c, uint id, uint n, uint sz) { // TODO : remove this (used by transpose only)
  off_t size = (off_t)sizeof(vec_t) + ((off_t) n) * sz;
  vec_t *vec = map_chunk (c, id, size);
  vec->count = n;  
  vec->limit = 0;
  vec->esize = sz;  
  vec->file = 2;
  return vec->data;
}

uint len_vec (coll_t *M, uint id) { 
  if (!has_vec(M,id)) return 0;
  void *V = get_vec(M,id); 
  uint n = len(V); 
  free_vec(V); 
  return n; 
}

/*
void *next_vec (coll_t *c, uint *id) {
  *id = c->next ? c->next [*id] : (1+*id) % len(c->offs);
  return get_vec (c, *id); 
}
*/

// copy the matrix (defragmenting it in the process)
void defrag_coll (char *SRC, char *TRG) {
  coll_t *S = open_coll (SRC, "r+"), *T = open_coll (TRG, "w+");
  uint id;
  for (id = 1; id <= nvecs(S); ++id) {
    vec_t *src = get_chunk(S,id);
    if (!src) continue;
    off_t size = (off_t)sizeof(vec_t) + (off_t) src->count * src->esize;
    put_chunk (T,id,src,size);
  }
  free_coll (S);
  free_coll (T);
}


/* //// part of old put_blob   ////
  off_t *offs = c->offs, next_offs = offs [c->ordr[id].next];
  // blob doesn't exist or doesn't fit at current position
  if (!offs[id] || offs[id] + size > next_offs) {
    if (offs[id]) del_blob (c,id); // delete if already exists
    if (id+1 >= len(L)) L = resize_vec (L, id+1);
    ord_t *this = L + id, *next = L + next_id, *prev = L + next->prev;
    this->next = next_id;           //         this -> next
    this->prev = next->prev;        // prev <- this
    next->prev = id;                //         this <- next
    prev->next = id;                // prev -> this
    return L;

    c->ordr = ord_ins (c->ordr, id, 0); // insert as last vec

    offs[id] = offs[0]; // offset = end of area
    offs[0] += size; // shift end of area
    expand_mmap (c->vecs, offs[0]); // update underlying file
  }
  void *trg = move_mmap (c->vecs, offs[id], size);
  memcpy (trg, blob, size);
 
}
*/


/*
void m_layout (matrix_t *M) {
  printf("# %d vecs, %d hash keys\n", m_count(M), hcount(M->ids));
  mindex_t *m, *root = M->indx;
  for (m = root; 1; m = root + m->next) {
    printf (" %10d %15lld %15lld\n", (m-root), m->offs, m->size);
    if (m->next == 0) break;
  } 
}

void m_visualize (matrix_t *M) {
  printf("# %d vecs, %d hash keys\n", m_count(M), hcount(M->ids));
  unsigned i;
  for (i = 1; i < vcount(M->indx); ++i) {
    if (!_ok(M,i)) continue;
    unsigned *vec = m_get_vec(M, i, 0, 0);
    printf (" %4d %10s: %d x %d / %d \n", i, i2key(M->ids,i), vesize(vec), vcount(vec), vtotal(vec));
    //for (j = 0; j < vcount(vec); ++j) printf (" %2d", vec[j]);
    //printf ("\n");
  }
}
*/





//#define _ok(m,i) ((i>0) && (i<vtotal(m->indx)) && (m->indx[i].offs || (i==m->indx[0].next)))

//#define m_get_vec(m,id,n,s) (_ok(m,id) ? m_resize_vec(m,id,n) : m_new_vec(m,id,n,s))

//#define m_chk_vec(v,m,id,n) ((n<=vtotal(v)) ? v : m_resize_vec(m,id,2*n))
/*
// fetch vector making sure at has space for n total entries
void *m_get_vec(matrix_t *m, unsigned id, unsigned n, unsigned s) {
  return _ok(m,id) ? m_resize_vec(m,id,n) : m_new_vec(m,id,n,s); }

void *m_chk_vec(void *v, matrix_t *m, unsigned id, unsigned n) {
  return (n <= vtotal(v)) ? v : m_resize_vec(m,id,2*n); }

// fetch vector making sure at has space for n new entries
void *m_app_vec(matrix_t *m, unsigned id, unsigned n, unsigned s) {
  if (!_ok(m,id)) return m_new_vec(m,id,n,s);
  void *vec = m_resize_vec (m,id,0); // get vector without resizing
  if (vcount(vec)+n > vtotal(vec)) // cannot fit n new entries?
    vec = m_resize_vec (m, id, vcount(vec)+n); // resize exactly
  return vec;
}

// time when the matrix was last modified
time_t m_time (char *path) {return file_modified (cat(path,"/data.head"));}

matrix_t *m_new2 (char *_path, unsigned nvecs, off_t mmap_size) {
  char *path = strdup(_path);
  matrix_t *m = vnew (1, sizeof (matrix_t), 0);
  int fd;
  mkdir (path, S_IRWXU | S_IRWXG | S_IRWXO);
  fd = safe_open (cat (path,"/data.vecs"), O_RDWR | O_CREAT | O_TRUNC);
  safe_truncate (fd, mmap_size);
  close(fd);
  m->ids = hnew (1+nvecs, cat (path,"/ids"));
  m->vecs = new_mmap (cat (path,"/data.vecs"), mmap_size, write_shared);
  m->indx = vnew (1+nvecs, sizeof (mindex_t), cat(path,"/data.head"));
  vcount(m->indx) = 1;
  m->indx[0].offs = MIN_OFFS;
  m->indx[0].prev = 0;
  m->indx[0].next = 0;  
  free(path);
  return m;
}

matrix_t *m_new (char *_path, unsigned nvecs) { return m_new2 (_path, nvecs, (1<<28)); }

matrix_t *m_open2 (char *_path, access_t access, off_t map_size) {
  char *path = strdup(_path);
  matrix_t *m = vnew (1, sizeof (matrix_t), 0);
  if (map_size == 0) map_size = (1<<28);
  m->ids = hmmap (cat (path,"/ids"), access);
  m->indx = vmmap (cat (path,"/data.head"), access);
  m->vecs = new_mmap (cat (path,"/data.vecs"), map_size, access);
  free(path);
  return m;
}

matrix_t *m_open (char *_path, access_t access) { return m_open2 (_path, access, 0); }

void m_free (matrix_t *m) {
  free_mmap (m->vecs); // unmap region, close file, free struct
  hfree (m->ids);
  vfree (m->indx);
  vfree (m);
}

// delete a vector from the matrix, 
// i.e. remove id from the m->indx, connect: prev <-> next
// note: this does not touch the contents, just the index
void m_del_vec (matrix_t *m, unsigned id) {
  mindex_t *this, *next, *prev;
  if (!_ok(m,id)) assert(0 && "cannot delete non-existent vector");
  this = m->indx + id;
  prev = m->indx + this->prev; // prev <-> this 
  next = m->indx + this->next; //          this <-> next
  prev->next = this->next;     // prev      ->      next
  next->prev = this->prev;     // prev      <-      next
  this->offs = this->size = this->prev = this->next = 0; 
}

// create a new vector 'id' with 'n' elements of size 'esize'
void *m_new_vec (matrix_t *m, unsigned id, unsigned n, unsigned esize) {
  mindex_t *root, *this, *last, *first;
  vector_t *result = NULL;
  off_t vsize = align8 (n * esize + sizeof (vector_t));
  //printf("   NEW: %d %d %d ", id, n, esize);
  assert (m && m->indx && m->vecs && (id>0));
  m->indx = vcheck (m->indx, id+1); // make sure index large enough
  if (id >= vcount (m->indx)) vcount (m->indx) = id+1;
  root  = m->indx + 0; 
  this  = m->indx + id;
  last  = m->indx + root->prev; // last <-> root
  first = m->indx + root->next; //          root <-> first
  // add new vector after last
  last->next = id;              // last -> this
  this->prev = root->prev;      // last <- this
  this->next = 0;               //         this -> root
  root->prev = id;              //         this <- root
  this->offs = root->offs;
  this->size = vsize;
  root->offs += vsize;
  mmap_ensure_size (m->vecs, root->offs);
  result = (vector_t *) mapped_region (m->vecs, this->offs, vsize);
  result->count = 0;
  result->alloc = n;
  result->esize = esize;
  result->file  = -2; // otherwise vresize() will wreak havoc
  vzero (result->data); // zero out unused portion
  //printf(" taking up %d ... %d\n", this->offs, root->offs);
  return result->data;
}

// resize vector 'id' s.t. it has space for 'n' elements
void *m_resize_vec (matrix_t *m, unsigned id, unsigned n) {
  mindex_t *root, *this, *next, *prev;
  off_t vsize, esize;
  vector_t *vec;
  assert (m && m->indx && m->vecs && _ok(m,id));
  root = m->indx + 0;
  this = m->indx + id; // index node for vector 'id'
  vec = (vector_t *) mapped_region (m->vecs, this->offs, this->size);
  if (n <= vec->alloc) return vec->data; // vector big enough
  //printf("resize: %d %d ... ", id, n);
  esize = vec->esize;
  vsize = align8 (n * esize + sizeof (vector_t)); // new size
  next = m->indx + this->next; // next vector in storage file
  prev = m->indx + this->prev; // previous vector in file
  // enough space between this and next vector => use that space
  if (vsize <= (next->offs - this->offs)) {
    //printf("nothing to do\n");
    vec = (vector_t *) mapped_region (m->vecs, this->offs, vsize);
    this->size = vsize;
    vec->alloc = n;
    vzero (vec->data); // zero out unused portion
  } // do nothing
  // this is last vector in a file => simply extend the file
  else if (id == root->prev) { 
    //printf("extending EOF from %d to %d\n", root->offs, this->offs + vsize);
    root->offs = this->offs + vsize; // new end of file
    mmap_ensure_size (m->vecs, root->offs); 
    vec = (vector_t *) mapped_region (m->vecs, this->offs, vsize);
    this->size = vsize;
    vec->alloc = n;
    vzero (vec->data); // zero out unused portion
  }
  // enough space between previous and next => move to the left
  else if ((next->offs > (MIN_OFFS + vsize)) && 
	   (next->offs - vsize) > (prev->offs + prev->size)) {
    off_t size = this->size;
    vector_t *tmp = malloc (size); // temp space
    //printf("moving left from %d to %d\n", this->offs, next->offs-vsize);
    memcpy (tmp, vec, size); // copy of the vector
    this->offs = next->offs - vsize; // amount of space we need
    vec = (vector_t *) mapped_region (m->vecs, this->offs, vsize);
    memcpy (vec, tmp, size);
    free (tmp);
    this->size = vsize;
    vec->alloc = n;
    vzero (vec->data); // zero out unused portion
  }
  else { // no space => create a new vector, move contents there
    off_t size = this->size;
    vector_t *tmp = malloc (size); // temporary space
    //printf("moving right from %d to EOF\n", this->offs);
    memcpy (tmp, vec, size); // create a copy of the vector
    m_del_vec (m, id); // delete old vector from the matrix
    vec = vstruct (m_new_vec (m, id, n, esize)); // new place for vec
    memcpy (vec, tmp, size); // copy contents to new place
    free (tmp); // release space used by the copy
  this->size = vsize;
  vec->alloc = n;
  vzero (vec->data); // zero out unused portion
  }
  this->size = vsize;
  vec->alloc = n;
  vzero (vec->data); // zero out unused portion
  return vec->data;
}

void *m_read_vec (matrix_t *m, unsigned id) {
  assert (_ok(m,id));
  int fd = m->vecs->file; mindex_t *indx = m->indx + id;
  safe_lseek (fd, indx->offs, SEEK_SET);
  //return vreadF (m->vecs->file);
  vector_t *v = malloc (indx->size);
  safe_read (fd, v, indx->size);
  v->file = -1; // vector in memory
  return v->data;
  //  ssize_t nr = pread (m->vecs->file, v, indx->size, indx->offs);
  //  if (nr == -1) {perror("[m_read_vec -> pread] failed"); assert(0);}
  //  return v->data;
}

void *m_mmap_vec (matrix_t *m, unsigned id) {
  assert (_ok(m,id));
  mindex_t *indx = m->indx + id;
  vector_t *v = mmap_region (m->vecs->file, indx->offs, indx->size, read_only);
  return v->data;
}

void m_unmap_vec (void *vec, matrix_t *m, unsigned id) {
  mindex_t *indx = m->indx + id;
  unmap_region (vstruct(vec), indx->offs, indx->size);
}

void *m_put_vec (matrix_t *M, unsigned id, void *vec) {
  void *result = m_get_vec (M, id, vcount(vec), vesize(vec));
  memcpy (result, vec, vcount(vec) * vesize(vec));
  vcount(result) = vcount(vec);
  return result;
}
*/

/*

// fetch vector 'id' from matrix, assume it exists
void *m_get_existing_vec (matrix_t *m, unsigned id) {
  unsigned size;
  mindex_t *this = m->indx + id;
  mindex_t *next = m->indx + this->next;
  vector_t *vec;
  this = m->indx + id;
  next = m->indx + this->next;
  size = next->offs - this->offs;
  vec = (vector_t *) mapped_region (m->vecs, this->offs, size);
  return vec->data;
}

void *m_vcheck (matrix_t *m, unsigned id, unsigned min_size) {
  void *vec = m_get_vec (m, id), *dup;
  if (min_size < vtotal(vec)) return vec; // vector big enough
  else {
    unsigned vsize = align8 (vsizeof (v));
    mindex_t *root = m->indx;
    mindex_t *this = m->indx+id;
    mindex_t *next = m->indx + this->next;
    if (next->offs > (this->offs + vsize)) { // got spare space 
      unsigned space = next->offs - (this->offs + vsize);
      vtotal(v) += space / vesize(v); // how many new elements
      if (min_size < vtotal(v)) return v;
    }
    if (id == root->prev) { // last vec in a file, just extend it
      vtotal(v) = 2 * min_size;
      root->offs = this->offs + align8 (vsizeof (v));
      mmap_ensure_size (m->vecs, root->offs);
      return m_get_vec (m, id); // need to remap the vector
    }
    if (1) {
      dup = vdup(vec);
      vec = m_new_vec (m, id, min_size, vesize(dup));
      memcpy (vec, dup, vcount(dup) * vesize(dup));
      vcount (vec) = vcount(dup);
      vfree (dup);
    }
    else { 
      // if we assume m_new_vec() doesn't touch contents, we could
      // do the above a bit more efficiently, perhaps something like:
      int fd = m->vecs->file;
      mindex_t *this = m->indx + id;
      unsigned offs = m->indx[id].offs + sizeof(vector_t);
      unsigned total=vtotal(vec), count=vcount(vec), esize=vesize(vec); 
      dup = safe_mmap (fd, offs, count * esize, read_only);
      vec = m_new_vec (m, id, min_size, esize);
      memcpy (vec, dup, count * esize); // this may be a real mess
      vcount (vec) = count;
      munmap (dup, cout * esize);
    }
  }
  
}
*/

// used to be in m_del_vec, now subsumed by m_new_vec()
//if (this->prev) { // if prev is a real vector (this isn't first)
//  unsigned space = (next->offs - this->offs); // add space to prev
//  void *d = m_get_vec (m, this->prev); 
//  vtotal(d) += space / vesize(d); // how many elements fit in space
//  memset (d + vcount(d) * vesize(d), 0, space); // zero out space
//} // if this is first, unused space will be grabbed by m_new_vec()

/* void *matr_vcheck (matrix_t *m, unsigned id, unsigned min_size) { */
/*   mindex_t *this = m->indx + id; */
/*   mindex_t *next = m->indx + this->next; */
/*   mindex_t *prev = m->indx + this->prev; */
/*   mindex_t *last = m->indx + _last(m); */
/*   void *vec = matr_get (m, id); */
/*   if (!vec) return NULL; // vector not in the matrix */
/*   if (min_size < vtotal(vec)) return vec; // vector big enough */
/*   if (id != _last(m)) { // move to the end of file */
/*     prev->next = this->next; // link up previous and next nodes */
/*     next->prev = this->prev; */
/*     last->next = id; */
/*     this->next = 0; */
/*     this->prev = _last(m); */
/*     this->offs = _size(m); */
/*     _last(m)   = id; */
/*     _size(m)  += vsizeof(vec); */
    
/*   } */
  
/* } */

/* void *matr_put (matrix_t *m, unsigned id, void *vec) { */
/*   vector_t *p; */
  
/*   mindex_t *this = (mindex_t *) vnext ((void **) &m->indx); */
/*   mindex_t *end  = m->indx + 0; */
/*   mindex_t *last = m->indx + end->next; */
/*   this->offs = end->offs; */
/*   end->offs  = this->offs + vsizeof(vec); */
/*   this->next = 0; */
/*   last->next = (this - m->indx); */
/*   end->next  = last->next; */
/*   mmap_ensure_size (m->vecs, end->offs);  */
/*   p = (vector_t *) mapped_region (m->vecs, this->offs + vsizeof(vec)); */
/*   p->count = 0; */
/*   p->esize = el_size; */
/*   p->alloc = max_els; */
/*   p->file  = -2; // otherwise vresize() will wreak havoc */
/*   return p->data; */
  
  
/* } */

/********************************************************************/
/********************************************************************/
/********************************************************************/
/*
// Primitive matrix implementation. Each vector stored in a separate 
// file, files are arranged in a 4-level directory tree. Vector ids 
// are mapped to file names: 0xAAABBCCDD -> matrix/aa/bb/cc/dd
// Pros: simple, all vector operations work, allows 2gb x 2gb matrices
// Cons: creating and traversing directory structure is inefficient

char *mid2path (char *root, unsigned id) {
  static char *path = 0;
  char hex[10];
  if (!path) path = calloc (1000, sizeof(char));
  sprintf(hex,"%08X",id);
  sprintf(path,"%s/%2.2s/%2.2s/%2.2s/%2.2s", 
	  root, hex+0, hex+2, hex+4, hex+6); // note: can be reversed
  return path;
}

// mkdir all necessary components of dir1/dir2/dir3/file
// try to be efficient by starting at the deepest level
// and recursively progressing up the path if needed
void force_mkdir (char *dir) { 
  int status = mkdir (dir, S_IRWXU); // try to create dir
  if (status && (errno == ENOENT)) { // parent doesn't exist
    char *s = strrchr (dir, '/'); // find last "/"
    assert (s);
    *s = '\0'; // truncate to get the parent 
    force_mkdir (dir); // create parent by recursing 
    *s = '/'; // restore path
    status = mkdir (dir, S_IRWXU); // repeat attempt
  }
  if (status) { 
    fprintf (stderr, "[force_mkdir] failed to create %s: [%d] ", 
	     dir, errno);
    perror (""); 
    assert (0);
  }
}

void mkdir_parent (char *path) {
  char *s = strrchr (path, '/');
  assert (s);
  *s = '\0';
  if (!file_exists (path)) // parent does not exist
    force_mkdir (path);
  *s = '/';
}

void *mget_vec (char *root, unsigned id, access_t access) {
  char *path = mid2path (root, id);
  if (!file_exists (path)) return NULL;
  return vmmap (path, access); 
}

void *mnew_vec (char *root, unsigned id, unsigned n, unsigned esize) {
  char *path = mid2path (root, id);
  mkdir_parent (path);
  return vnew (n, esize, path);
}

void mput_vec (char *root, unsigned id, void *d) {
  vwrite (d, mid2path (root, id));
}
*/
/* used to test sequential reads and writes.
   current tests are random-access

     for (x = 0; x < X; ++x) {
      unsigned *a = (unsigned *) mget_vec (NAME, x, read_only);
      for (y = 0; y < Y; ++y) 
	if (a[y] != x + y) assert(0);
      vfree (a);
    }
    printf ("[%f] verified matrix read_only\n", vtime());

    for (x = 0; x < X; ++x) {
      unsigned *a = mnew_vec (NAME, x, 1, sizeof(unsigned));
      vfree (a); }
    for (y = 0; y < Y; ++y) {
      for (x = 0; x < X; ++x) {
	unsigned *a = mget_vec (NAME, x, write_shared);
	* (unsigned *) (vnext ((void **)&a)) = x + y;
	vfree (a);
      }
    }
    printf ("[%f] filled matrix by column\n", vtime());
    
    for (x = 0; x < X; ++x) {
      unsigned *a = (unsigned *) mget_vec (NAME, x, write_shared);
      for (y = 0; y < Y; ++y) 
	if (a[y] != x + y) assert(0);
      vfree (a);
    }
    printf ("[%f] verified matrix write_shared\n\n", vtime());


  if (type == 2) { 
    printf ("Timings for a %d x %d matrix of type 2\n", X, Y);
    matrix_t *m;
    
    m = m_new (NAME, X);
    //m_visualize(m,0);
    for (x = 0; x < X; ++x) {
      unsigned *a = (unsigned *) m_new_vec (m, x+1, Y, sizeof(unsigned));
      vcount(a) = Y;
      for (y = 0; y < Y; ++y) 
	a[y] = x + y;
      * (unsigned *) (vnext ((void **)&a)) = x + y;    
    }
    //m_visualize(m,0);
    m_free (m);
    printf ("[%f] filled write_shared matrix\n", vtime());
    
    m = m_open (NAME, read_only);
    for (x = 0; x < X; ++x) {
      unsigned *a = (unsigned *) m_get_vec (m, x+1, 0, 0);
      for (y = 0; y < Y; ++y) 
	if (a[y] != x + y) assert(0);
    }
    m_free (m);
    printf ("[%f] verified matrix read_only\n", vtime());

    m = m_new (NAME, X);
    //m_visualize(m,0);
    for (x = 0; x < X; ++x) {
      unsigned *a = (unsigned *) m_new_vec (m, x+1, 1, sizeof(unsigned));
    }
    for (y = 0; y < Y; ++y) {
      for (x = 0; x < X; ++x) {
	unsigned *a = m_get_vec (m,x+1,y+1,0);
	if (vcount(a) <= y) vcount(a) = (y+1);
	a[y] = x + y;
      }
      * (unsigned *) (vnext ((void **)&a)) = x + y;    
    }
    //m_visualize(m,0);
    m_free (m);
    printf ("[%f] filled write_shared with realloc\n", vtime());
        
    m = m_open (NAME, write_shared);
    for (x = 0; x < X; ++x) {
      unsigned *a = (unsigned *) m_get_vec (m, x+1, 0, 0);
      for (y = 0; y < Y; ++y) 
	if (a[y] != x + y) assert(0);
    }
    m_free (m);
    printf ("[%f] verified matrix write_shared\n\n", vtime());
    
  }

*/

//inline static void *add_chunk_inmem (coll_t *c, uint id, off_t size) {
//  if (id >= len(c->offs)) c->offs = resize_vec (c->offs, id+1);
//  c->offs[id] = realloc (NULL + c->offs[id], size) - NULL;
//  return NULL + c->offs[id]; }

/*
inline void *get2_chunk (coll_t *c, uint id) {
  if (!has_vec(c,id)) return NULL;
  uint next = c->next ? c->next[id] : (id+1) % len(c->offs);
  void *buf = safe_malloc (c->offs[next] - c->offs[id]);
  safe_pread (c->vecs->file, buf, c->offs[next] - c->offs[id], c->offs[id]);
  return buf;
} 
*/

/*
// append a new chunk to end of the collection and link id to it
// do not use directly on collections opened as 'a+' or 'w+'
// returned chunk is not initialised in any manner
inline void *add_chunk (coll_t *c, uint id, off_t size) {
  if (!c->path) return add_chunk_inmem (c,id,size);
  mov_chunk (c, id, size);
  return move_mmap (c->vecs, c->offs[id], size);
}

// insert a new chunk into collection and link id to it
// use only on collections opened as 'a+' or 'w+'
// data in existing chunks is retained
inline void *ins_chunk (coll_t *c, uint id, off_t size) {
  if (!c->path) return add_chunk_inmem (c,id,size);
  assert (c && id && size);
  off_t old = mov_chunk (c, id, size);
  off_t offs = c->offs [id], next = c->offs [c->next[id]];
  if (offs && offs+size <= next) // exists and fits in its current slot
    return move_mmap (c->vecs, offs, size); 
  if (offs) del_chunk (c, id); // doesn't fit => delete it, add at the end
  uint head = 0, tail = c->prev[head]; // tail <------> head
  c->next[id] = head;                  //         id -> head
  c->prev[head] = id;                  //         id <- head
  c->prev[id] = tail;                  // tail <- id
  c->next[tail] = id;                  // tail -> id
  void *trg = add_chunk (c, id, size);
  if (offs) safe_pread (c->vecs->file, trg, next-offs, offs); // copy old contents
  return trg;
}
*/

/*
inline void *get2_vec (coll_t *c, uint id) {
  vec_t *hdr = get2_chunk (c, id);
  if (!hdr) return NULL;
  hdr->file = 0; // in-memory
  return hdr->data;
}
*/

/*
// return pointer to contents of vector number 'id'
// if it doesn't exist -- create with n elements of size sz
// if it exists -- add n additional elements (initialize to 0)
//              -- physically resize to next power of 2
void *init_vec_t (vec_t *v, uint n, uint s, int fd) ;
inline void *mmap_vec (coll_t *c, uint id, uint n, uint sz) {
  if (!has_vec(c,id)) { // vector doesn't exist => create a new one
    vec_t *vec = ins_chunk (c, id, sizeof(vec_t) + n*sz);
    return init_vec_t (vec, n, sz, 2); 
  }
  vec_t *vec = move_mmap (c->vecs, c->offs[id], sizeof(vec_t));
  off_t old_size = vsizeof(vec);
  assert (old_size < (1u<<31));
  if (vec->count + n <= vlimit(vec)) { // vec big enough => just return it
    if (n) vec->count += n;
    vec = move_mmap (c->vecs, c->offs[id], old_size);
    return vec->data;
  }
  vec->count += n;
  vec->limit = ilog2 (vec->count-1) + 1; 
  off_t new_size = vsizeof(vec);
  vec = ins_chunk (c, id, new_size); // will retain existing data
  memset ((void*)vec + old_size, 0, new_size - old_size);
  return vec->data;
}
*/

/*
void free_ptrs (void **ptrs) {
  void **p = ptrs-1, **end = ptrs+len(ptrs);
  while (++p < end) if (*p) free (*p);
  memset (ptrs, 0, len(ptrs)*sizeof(void*));
  free_vec (ptrs);
}
*/

/*

inline void put_chunk2 (coll_t *c, uint id, void *buf, off_t size) {
  if (!c->path) return put_chunk_inmem (c,id,buf,size);
  mov_chunk (c, id, size);
  safe_pwrite (c->vecs->file, buf, size, c->offs[id]);
}
*/
