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

#include "mmap.h"

#ifndef VECTOR
#define VECTOR

typedef struct {
  unsigned count; // number of elements in the vector
  unsigned limit:10; // number of elements allocated (log2-scale), 0 = count
  unsigned esize:10; // element size (in bytes)
  unsigned file:12; // descriptor, except 0=ram, 1=read-only, 2=in-a-matrix
  char data [0];
} vec_t; // total size=64bit

#define vect(d) ((vec_t *) ((char *) (d) - sizeof(vec_t)))
#define len(d) (vect(d)->count)
#define vesize(d) (vect(d)->esize)
#define vlimit(v) ((v)->limit ? (1lu << (v)->limit) : (v)->count)
#define vsizeof(v) ((off_t)vlimit(v) * (v)->esize + (off_t)sizeof(vec_t))
#define vresize(d,n) ((n) < vlimit(vect(d)) ? d : resize_vec(d,n))

/*
inline vec_t *vect (void *d) {return (d - sizeof(vec_t));}
inline uint len(void *d) {return vect(d)->count;}
inline uint vesize(void *d) {return vect(d)->esize;}
inline uint vlimit(vec_t *v) {return v->limit ? (1u<<v->limit) : v->count;}
inline off_t vsizeof(vec_t *v) {return sizeof(vec_t) + vlimit(a)*a->esize;}
*/

/*
inline void* vgrow (void *x, uint n) {
  vec_t *v = vect(x);
  return (v->count + n < vlimit(v)) ? x : _agrow (x, n); }

inline void* vresize (void *x, uint n) {
  return (n < vlimit(vect(x))) ? x : _vresize (x,n);
}
*/

void   *open_vec_if_exists (char *path, char *access) ;
void   *open_vec (char *path, char *access, uint esize) ;
void    *new_vec (uint n, uint esize) ;
void    free_vec (void *d) ;
void   free_vecs (void *v1, ...) ;
void   write_vec (void *d, char *path) ;
void   *read_vec (char *path) ;
void   *copy_vec (void *d) ;
void *resize_vec (void *d, uint n) ;
void *new_or_resize_vec (void *d, uint n, uint esz);
void *append_vec (void *d, void *el) ;
void *append_many (void *vec, void *els, uint k) ; // append k els at the end of vec
void *prepend_vec (void *vec, void *els, uint k) ; // prepend k els at the beginning

void reverse_vec (void *V) ; // reverse elements in-place
void    sort_vec (void *d, int (*cmp) (const void *, const void *)) ;

void *bsearch_vec (void *vec, uint id); // pointer to first el >= id

void find_el (void *el, void *vec, int (*cmp) (const void *, const void *)) ;
void *vec_el (void **vec, uint n) ;

void *set_vec_el (void *vec, uint i, void *el) ;
void *ref_vec_el (void **vec, uint i) ;
void *ins_vec_el (void *vec, uint i, void *el) ; // insert el into [i], shift old [i..n]
void del_vec_el (void *vec, uint i) ;
void **pointers_to_vec (void *V) ; // vector of n pointers

int cmp_jix (const void *n1, const void *n2) ; // by increasing j then i
int cmp_jix_X (const void *n1, const void *n2) ; // by decreasing x
int cmp_jix_i (const void *n1, const void *n2) ; // by increasing i then j
int cmp_jix_jX (const void *n1, const void *n2) ; // incr. j then decr. X

int cmp_it_i (const void *n1, const void *n2) ; // by increasing i
int cmp_it_t (const void *n1, const void *n2) ; // by increasing t
int cmp_ix_i (const void *n1, const void *n2) ; // by increasing i
int cmp_ix_I (const void *n1, const void *n2) ; // by decreasing i
int cmp_ix_x (const void *n1, const void *n2) ; // by increasing x
int cmp_ix_X (const void *n1, const void *n2) ; // by decreasing x
int cmp_ixy_i (const void *n1, const void *n2) ; // by increasing id
int cmp_ixy_x (const void *n1, const void *n2) ; // by increasing x
int cmp_ixy_y (const void *n1, const void *n2) ; // by increasing y
int cmp_ixy_X (const void *n1, const void *n2) ; // by decreasing x
int cmp_ixy_Y (const void *n1, const void *n2) ; // by decreasing y
int cmp_xy_x (const void *n1, const void *n2) ; // by increasing x
int cmp_xy_X (const void *n1, const void *n2) ; // by decreasing x
int cmp_x (const void *n1, const void *n2) ; // increasing float
int cmp_X (const void *n1, const void *n2) ; // decreasing float
int cmp_u (const void *n1, const void *n2) ; // increasing uint
int cmp_U (const void *n1, const void *n2) ; // decreasing uint
int cmp_str (const void *a, const void *b) ; // strings, lexically
int cmp_ijk_i (const void *n1, const void *n2) ; // by increasing i
int cmp_ijk_j (const void *n1, const void *n2) ; // by increasing j
int cmp_ijk_k (const void *n1, const void *n2) ; // by increasing k
int cmp_sjk_s (const void *n1, const void *n2) ; // lexically
int cmp_sjk_j (const void *n1, const void *n2) ; // increasing start
int cmp_sjk_k (const void *n1, const void *n2) ; // increasing stop

int cmp_ulong_ptr (const void *_a, const void *_b) ; // by increasing *ulong
int cmp_Ulong_ptr (const void *_a, const void *_b) ; // decreasing


void **new_2D (uint rows, uint cols, uint esize) ; // X[i][j]
void ***new_3D (uint deep, uint rows, uint cols, uint esize) ; // X[i][j][k]
void free_2D (void **X) ;
void free_3D (void ***X) ;

uint triang (uint i, uint j) ; // (i,j) --> offset in lower-triangular
uint trilen (uint o) ; // triangular offset -> number of rows & columns

#endif

//inline void *vnext (void **d) ;
//void vzero (void *d) ;

//#define vtotal(d)  ((unsigned)1 << vstruct(d)->alloc)
//#define vesize(d)  (vstruct(d)->esize)
//#define vsizeof(d) (vtotal (d) * vesize(d) + sizeof (vstruct_t))
//#define vend(d) ((d) + len(d))

//#ifdef __APPLE__
// Apple uses the name vfree in one of its libraries and the linker gives us grief...
//#define vfree our_vfree_method
//#endif

/// causes problems because arguments end up evaluated twice
///#define vcheck(d,s) (((s) < vtotal(d)) ? (d) : vresize (d, 2 * (s)))
///#define vnext(d) ((*d = vcheck (*d, ++vcount (*d))) + vesize(*d) * (vcount(*d) - 1))

/////////////////// 07.13.99 Experimental...
//void *vmerge (unsigned result_esize,
//	      void *vector1, void *vector2,
//	      void *parameter,
//	      int (*compare) (void*, void*),
//	      void (*join) (void*,void*,void*,void*));

