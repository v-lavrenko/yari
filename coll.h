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

#include "vector.h"

#ifndef COLL
#define COLL

#define COLL_VERSION 20130808 

// TODO: replace vecs with int fd + bump VERSION
typedef struct {
  off_t  *offs; // offs[id] = offset of id'th vector in vecs (or pointer)
  uint   *prev; // prev[id] = vec that precedes id in vecs, 0 if id is 1st
  uint   *next; // next[id] = vec that follows id in vecs, 0 if id is last
  mmap_t *vecs; // window containing vectors 
  char   *path; // path where the matrix exists on disk
  char *access; // r,w,a
  uint version; 
  uint    rdim; // number of rows
  uint    cdim; // number of columns
} coll_t;

#define nvecs(c) (len((c)->offs)-1)

coll_t *open_coll (char *_path, char *access) ;
coll_t *reopen_coll (coll_t *c, char *access) ;
void free_coll (coll_t *c) ;
void free_colls (coll_t *c1, ...) ;
void rm_coll (coll_t *c) ;
void empty_coll (coll_t *M) ;
int coll_exists (char *path) ;
time_t coll_modified (char *path) ;
coll_t *open_coll_if_exists (char *path, char *access) ;
coll_t *open_coll_inmem () ;

coll_t *copy_coll (coll_t *src) ;

void *get_chunk (coll_t *c, uint id) ;
void del_chunk (coll_t *c, uint id) ;
void put_chunk (coll_t *c, uint id, void *chunk, off_t size) ;
off_t chunk_sz (coll_t *c, uint id) ;

#define has_vec(c,i) ((i) > 0 && (i) <= nvecs(c) && (c)->offs[(i)])
void *get_vec (coll_t *c, uint id) ;
void del_vec (coll_t *c, uint id) ;
void put_vec (coll_t *c, uint id, void *vec) ;
void *next_vec (coll_t *c, uint *id);
void *get_or_new_vec (coll_t *c, uint id, uint esize);
void *get_vec_ro (coll_t *c, uint id) ;
void *get_vec_mp (coll_t *c, uint id) ;
uint len_vec (coll_t *M, uint id) ;

void defrag_coll (char *SRC, char *TRG) ;

/*

typedef struct {
  off_t offs; // offset in file where vector is located
  uint prev; // id of the vector that precedes in file (0 if first)
  uint next; // id of the vector that follows in file (0 if last)
} offs_t;

typedef struct {
  uint prev; // id of the vector that precedes in file (0 if first)
  uint next; // id of the vector that follows in file (0 if last)
} ord_t;

#define _ok(m,i) ((i>0) && (i<vtotal(m->indx)) && (m->indx[i].offs))
#define _ok_chain(r,id) (((r+((r+id)->next))->prev == id) && ((r+((r+id)->prev))->next == id))
#define m_count(m) (vcount(m->indx)-1)

matrix_t *m_new (char *path, unsigned nvecs) ; // create a new matrix
matrix_t *m_new2 (char *path, unsigned nvecs, off_t mmap_size) ; // create a new matrix
matrix_t *m_open (char *path, access_t access) ; // mmap existing one
matrix_t *m_open2 (char *path, access_t access, off_t map_size) ; 
void m_free (matrix_t *m) ; // deallocate matrix, flush to disk
void m_defrag (char *source, char *target) ; // defragment matrix
void m_visualize (matrix_t *M) ; // print summary of vectors in matrix
void m_layout (matrix_t *M) ; // print layout of vectors on disk
time_t m_time (char *path) ; // last time the matrix was modified

// create a new vector 'id' with 'n' slots of size 'esize'
void *m_new_vec (matrix_t *m, unsigned id, unsigned n, unsigned esize) ;
// make sure vector 'id' has at least 'n' slots
void *m_resize_vec (matrix_t *m, unsigned id, unsigned n) ;
// remove vector 'id'
void m_del_vec (matrix_t *m, unsigned id) ; 
// get vector 'id', making sure it has at least 'n' slots
void *m_get_vec(matrix_t *m, unsigned id, unsigned n, unsigned s) ;
// make sure 'v' (stored as 'id') has at least 'n' slots
void *m_chk_vec(void *v, matrix_t *m, unsigned id, unsigned n) ;
// store vector 'vec' in the matrix 'm' under 'id', return
void *m_put_vec(matrix_t *m, unsigned id, void *vec) ;
// get vector 'id', ensuring space for 'n' new slots
void *m_app_vec(matrix_t *m, unsigned id, unsigned n, unsigned s) ;
// circumvent mmap, read directly from file into malloc()ed buffer
void *m_read_vec(matrix_t *m, unsigned id) ;
void *m_mmap_vec(matrix_t *m, unsigned id) ;
void m_unmap_vec(void *vec, matrix_t *m, unsigned id) ;


// older implementation: store each vector in a separate file
// mmap vector 'id' from 'path' either 'read_only' or 'write_shared'
void *mget_vec (char *path, unsigned id, access_t access) ;
// create a new vector 'id' with 'n' slots of size 'esize'
void *mnew_vec (char *path, unsigned id, unsigned n, unsigned esize);
// store vector 'd' as 'id' in 'path'
void mput_vec (char *path, unsigned id, void *d) ;

// creates a directory and all required parents
void force_mkdir (char *dir);
*/

#endif
