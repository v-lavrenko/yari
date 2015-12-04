/*

   Copyright (C) 1997-2014 Victor Lavrenko

   All rights reserved. 

   THIS SOFTWARE IS PROVIDED BY VICTOR LAVRENKO AND OTHER CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
   COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
   OF THE POSSIBILITY OF SUCH DAMAGE.

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

#define vect(d) ((vec_t *) ((char *) (d) - sizeof (vec_t)))
#define len(d) (vect(d)->count)
#define vesize(d) (vect(d)->esize)
#define vlimit(v) ((v)->limit ? (1u << (v)->limit) : (v)->count)
#define vsizeof(v) ((off_t) (sizeof(vec_t) + vlimit(v) * (v)->esize))
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

void    *new_vec (uint n, uint esize) ;
void   *open_vec (char *path, char *access, uint esize) ;
void    free_vec (void *d) ;
void   write_vec (void *d, char *path) ;
void   *read_vec (char *path) ;
void   *copy_vec (void *d) ;
void *resize_vec (void *d, uint n) ;
void *append_vec (void *d, void *el) ;
void *append_many (void *vec, void *els, uint k) ;
void    sort_vec (void *d, int (*cmp) (const void *, const void *)) ;

void find_el (void *el, void *vec, int (*cmp) (const void *, const void *)) ;
void *vec_el (void **vec, uint n) ;

void *set_vec_el (void *vec, uint i, void *el) ;
void *ref_vec_el (void **vec, uint i) ;

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

