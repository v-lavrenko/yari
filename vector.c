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

#include "vector.h"

static inline void *init_vec_t (vec_t *v, uint n, uint s, int fd) {
  v->count = n;  v->limit = 0;
  v->esize = s;  v->file = fd;
  if (n && s) memset (v->data, 0, n * (size_t)s);
  return v->data;
}

void *new_vec (uint n, uint esize) {
  vec_t *v = safe_malloc (sizeof(vec_t) + n * (size_t)esize);
  return init_vec_t (v, n, esize, 0);
}

void *open_vec (char *path, char *access, uint esize) {
  int fd = safe_open (path, access);
  off_t old = safe_lseek (fd, 0, SEEK_END); // zero if new file
  off_t size = old ? old : safe_truncate (fd, sizeof(vec_t));
  vec_t *v = safe_mmap (fd, 0, size, access);
  if (v->file > 1) fprintf (stderr, "ERROR: %s is CORRUPTED, delete it\n", path); 
  if (!old) init_vec_t (v, 0, esize, fd); // new vector => init all elements
  else if (*access == 'r') close(fd); // release fd and still keep the mmap
  else v->file = fd; // replace with stored descriptor with real one
  assert (size == vsizeof(v));
  return v->data;
}

void free_vec (void *d) {
  if (!d) return;
  vec_t *v = vect (d);
  int fd = v->file;
  switch (fd) {
  case 0: free(v); break; // vector is mallocced => free it
  case 1: munmap (v, vsizeof(v)); break; // read-only
  case 2: break; // in a matrix => nothing to be done
  default: // mmapped in a file
    v->file = 1; // descriptor stored in file should always be 1
    munmap (v, vsizeof (v)); // unmap, flushing v to file
    close(fd);  // finally, close the real descriptor
  }
}

// resize vector to 'num' elements, re-allocate if needed
// vector will be physically resized to next power of 2
void *resize_vec (void *d, uint n) {
  if (!d) return NULL;
  vec_t *v = vect (d);
  //if (n && n < vlimit(v)) {v->count = n; return d;} // big enough already
  if (n <= vlimit(v)) { v->count = n; return d;} // big enough already
  uint fd = v->file;
  if (fd==1 || fd==2) assert (0 && "cannot resize read-only vectors");
  off_t old_size = vsizeof (v);
  //v->count = n ? n : v->count;
  v->count = n;
  //v->limit = n ? ilog2(n-1)+1 : 0; // new allocation (log-scale)
  v->limit = ilog2(n-1)+1; // new allocation (log-scale)
  off_t new_size = vsizeof (v);  
  if (fd == 0) v = safe_realloc (v, new_size); // in memory
  else v = safe_remap (fd, v, old_size, new_size); // in a file
  //uint a = vlimit(v), esz = v->esize, c = v->count;
  //if (a > c) memset (v->data + c*esz, 0, (a-c)*esz);
  memset ((void*)v + old_size, 0, new_size - old_size);
  return v->data;
}

// append an element to the end of vector
// use for adding a single element:
// cv_t el = {2, 1}; vec = append (vec, &el);
void *append_vec (void *vec, void *el) {
  if (!el || !vec) return vec;
  uint i = len(vec), sz = vesize(vec);
  vec = resize_vec (vec, i+1);
  memcpy (vec + i*sz, el, sz);
  return vec;
}

void *append_many (void *vec, void *els, uint k) {
  if (!els || !vec) return vec;
  uint n = len(vec), sz = vesize(vec);
  vec = resize_vec (vec, n+k);
  memcpy (vec + n*sz, els, k*sz);
  return vec;
}

void *set_vec_el (void *vec, uint i, void *el) {
  if (i >= len(vec)) vec = resize_vec (vec, i+1);
  uint sz = vesize(vec);
  memcpy (vec + i*sz, el, sz);
  return vec;
}

void *ref_vec_el (void **vec, uint i) {
  if (i > len(*vec)) (*vec) = resize_vec ((*vec), i+1);
  return (*vec) + i;
}

/*
void *grow_vec (void *d, uint n, void **next) {
  uint old_n = len(d), esz = vesize(d);
  d = resize_vec (d, n);
  *next = d + old_n * esz;
  return d;
}

void *next_el (void **d, uint n) {
  uint old_n = len(*d), esz = vesize(*d);
  *d = resize_vec (*d, n);
  return d + old_n * esz;
}

void trials {
  pair_t *element, *array;
  // ------------------------------ method 1
  element->x = x;
  element->y = y;
  array = append_element (array, element);
  // ------------------------------ method 2
  element = append_element (&array, 1);
  element->x = x;
  element->y = y;
  // ------------------------------ method 3  
  array = grow_list (array, 1, &element);
  element->x = x;
  element->y = y;
}
*/

// exact copy of vector (with no extra padding)
void *copy_vec (void *src) {
  if (!src) return NULL;
  uint n = len(src), sz = vesize(src);
  void *trg = new_vec (n, sz); // header initialised here
  memcpy (trg, src, n*sz); // copying just the contents
  return trg;
}

/*
// append vector B to the end of vector A
void *cat_vec (void *A, void *B) {
  uint nA = len(A), nB = len(B), esz = vesize(A), eszB = vesize(B);
  assert ((esz == eszB) && "cannot concatenate vectors of different types");
  A = resize_vec (A, nA+nB);
  memcpy (A + nA*esz, B, nB*esz);
  return A;
}
*/

/*
void *slice_vec (void *d, long beg, long end) {
  vec_t *v = vect(d);
  long N = v->count, sz = v->esize;
  beg = (beg <  0) ? (N-beg) : (beg > N) ? N : beg;
  end = (end <= 0) ? (N-end) : (end > N) ? N : end;
  end = (end < beg) ? beg : end;
  void *n = temp_vec ((end-beg), sz);
  memcpy (n, d + beg*sz, (end-beg)*sz);
  return n;
}
*/

void write_vec (void *d, char *path) {
  int fd = safe_open (path, "w");
  vec_t *v = vect(d);
  int file = v->file;
  if (file != 1) v->file = 1;
  safe_write (fd, v, vsizeof(v));
  if (file != 1) v->file = file;
  close (fd);
}

void *read_vec (char *path) {
  int fd = safe_open (path, "r");
  off_t size = safe_lseek (fd, 0, SEEK_END);
  vec_t *v = safe_calloc (size);
  safe_lseek (fd, 0, SEEK_SET);
  safe_read (fd, v, size);
  v->file = 0; // vector is in memory
  assert (size == vsizeof(v));
  close (fd);
  return v->data;
}

void sort_vec (void *d, int (*cmp) (const void *, const void *)) {
  qsort (d, len(d), vesize(d), cmp); }

void *bsearch_vec_old (void *el, void *vec, int (*cmp) (const void *, const void *)) {
  return bsearch (el, vec, len(vec), vesize(vec), cmp); }

// returns pointer to first element >= id
void *bsearch_vec (void *vec, uint id) { // assume id is 1st field in each element
  long lo = -1, hi = len(vec), mid, sz = vesize(vec); // both out of range
  while (hi - lo > 1) {
    mid = (lo + hi) / 2;
    uint *pid = (uint *) ((char*)vec + mid * sz);
    if (*pid < id) lo = mid; // lo <  id
    else           hi = mid; // hi >= id
  }
  return (char*)vec + hi * sz;
}

///////////////////////////// sorting for common vector types

int cmp_jix (const void *n1, const void *n2) { // by increasing j then i
  jix_t *r1 = (jix_t*) n1, *r2 = (jix_t*) n2; 
  uint di = r1->i - r2->i, dj = r1->j - r2->j;
  return dj ? dj : di; }

int cmp_it_i (const void *n1, const void *n2) { // by increasing i
  uint i1 = ((it_t*)n1)->i, i2 = ((it_t*)n2)->i; 
  return i1 - i2; }

int cmp_it_t (const void *n1, const void *n2) { // by increasing t
  uint t1 = ((it_t*)n1)->t, t2 = ((it_t*)n2)->t; 
  return t1 - t2; }

int cmp_ix_i (const void *n1, const void *n2) { // by increasing id
  uint i1 = ((ix_t*)n1)->i, i2 = ((ix_t*)n2)->i; 
  return i1 - i2; }

//int cmp_ix_x (const void *n1, const void *n2) { // by increasing value
//  float x1 = ((ix_t*)n1)->x, x2 = ((ix_t*)n2)->x;
//  return (x1 < x2) ? -1 : (x1 > x2) ? +1 : 0; }

int cmp_ix_x (const void *n1, const void *n2) { return -cmp_ix_X (n1,n2); }
int cmp_ix_X (const void *n1, const void *n2) { // by decreasing value
  float x1 = ((ix_t*)n1)->x, x2 = ((ix_t*)n2)->x;
  return (x1 > x2) ? -1 : (x1 < x2) ? +1 : 0; }

int cmp_ixy_i (const void *n1, const void *n2) { // by increasing id
  uint i1 = ((ixy_t*)n1)->i, i2 = ((ixy_t*)n2)->i; 
  return i1 - i2; }

int cmp_ixy_x (const void *n1, const void *n2) { return -cmp_ixy_X (n1,n2); }
int cmp_ixy_X (const void *n1, const void *n2) { // by decreasing x
  float x1 = ((ixy_t*)n1)->x, x2 = ((ixy_t*)n2)->x;
  return (x1 > x2) ? -1 : (x1 < x2) ? +1 : 0; }

int cmp_ixy_y (const void *n1, const void *n2) { return -cmp_ixy_Y (n1,n2); }
int cmp_ixy_Y (const void *n1, const void *n2) { // by decreasing y
  float y1 = ((ixy_t*)n1)->y, y2 = ((ixy_t*)n2)->y;
  return (y1 > y2) ? -1 : (y1 < y2) ? +1 : 0; }

int cmp_x (const void *n1, const void *n2) { return -cmp_X (n1,n2); }
int cmp_X (const void *n1, const void *n2) { return *((float*)n2) - *((float*)n1); }

/*
void vzero (void *d) { // zero out unused portion of the vector
  vstruct_t *v = vstruct(d); // between count and alloc
  unsigned c = v->count, a = vtotal(v), e = v->esize;
  if (c < a) memset (v->data + c*e, 0, (a-c)*e); 
}
*/

//////////////////////////////////////////////////
/// 07.13.99 EXPERIMENTAL...
/// Generic merge is like merge_wnodes (coll.c), 
/// but operates on vectors of arbitrary data types
/*
void *vmerge (unsigned result_esize, 
	      void *vector1, void *vector2, 
	      void *parameter,
	      int (*compare) (void*, void*),
	      void (*join) (void*,void*,void*,void*)){
  void *v1 = vector1, *v2 = vector2;
  void *v1end = v1 + len(v1) * vesize (v1);
  void *v2end = v2 + len(v2) * vesize (v2);
  void *result = new_vec (len(v1) + len(v2),
		       result_esize, 0);
  while (v1 < v1end && v1 < v2end) {
    int cmp = compare (v1, v2); 
    if (cmp == 0) { // elements have the same id
      join (result, v1, v2, parameter);
      v1 += vesize (v1);
      v2 += vesize (v2);
    }
    else if (cmp < 0) { // no pair for v1
      join (result, v1, NULL, parameter);
      v1 += vesize (v1);
    }
    else { // no pair for v2
      join (result, NULL, v2, parameter);
      v2 += vesize (v2);
    }
  }
  
  for (; v1 < v1end; v1 += vesize (v1))
    join (result, v1, NULL, parameter);
  
  for (; v1 < v1end; v1 += vesize (v1))
    join (result, NULL, v2, parameter);
  
  return result;
}

*/

// mmap_t *new_map (char *path, char access) {
//   mmap_t *map = calloc (1, sizeof (mmap_t));
//   int file_permissions = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; //  rw-r--r--
//   switch (access) {
//   case 'r': // open existing map for reading only
//     map->file_access = O_RDONLY; 
//     map->mmap_access = PROT_READ; 
//     break;
//   case 'w': // open existing map for reading and writing
//     map->file_access = O_RDWR; 
//     map->mmap_access = PROT_READ | PROT_WRITE;
//     break;
//   case 'n': // create a new map for reading and writing
//     map->file_access = O_RDWR | O_CREAT | O_TRUNC;
//     map->mmap_access = PROT_READ | PROT_WRITE;
//     break;
//   default: assert (0);
//   }
//   map->file = safe_open (path, map->file_access, file_permissions); 
//   if (map->file == -1) {
//     fprintf (stderr, "[new_map] could not open %s for '%c' access\n", path, access);
//     perror ("[new_map]"); fflush (stderr); assert (0);
//   }
//   map->alloc = safe_lseek (map->file, 0, SEEK_END);
//   assert ((access == 'n' && map->alloc == 0) || (access!='n' && map->alloc > 0));
//   if (map->alloc == 0) return map;
//   map->data = (char *) safe_mmap 
//     (0, map->alloc, map->mmap_access, MAP_SHARED, map->file, 0);
//   if (map->data == (char *) -1) {
//     fprintf (stderr, "[new_map] could not map %d bytes in %s\n", map->alloc, path);
//     perror ("[new_map]"); fflush (stderr); assert (0);
//   }
//   safe_lseek (map->file, 0, SEEK_SET);
//   return map;
// }

// unsigned alloc_in_map (mmap_t *map, unsigned need) {
//   if (map->alloc <= map->used + need) {
//     if (map->data) munmap (map->data, map->alloc);
//     map->alloc = 2 * (map->alloc + need); 
//     lseek (map->file, map->alloc, SEEK_SET);
//     if (1 != write (map->file, "\0", 1)) {
//       fprintf (stderr, "[alloc_in_map] could not extend file to %d bytes\n", map->alloc);
//       perror ("[alloc_in_map]"); fflush (stderr); assert (0);
//     }
//     map->data = (char *) safe_mmap 
//       (0, map->alloc, map->mmap_access, MAP_SHARED, map->file, 0);
//     if (map->data == (char *) -1) {
//       fprintf (stderr, "[alloc_in_map] could not remap %d bytes\n", map->alloc);
//       perror ("[alloc_in_map]"); fflush (stderr); assert (0);
//     }
//   }
//   map->used += need;
//   return map->used - need;
// }

// void free_map (mmap_t *map) {
//   if (map->data) munmap (map->data, map->alloc);
//   close (map->file);
//   memset (map, 0, sizeof (mmap_t));
//   free (map);
// }

// void *vector (unsigned num_els, unsigned el_size, char *path, char *ext) {
//   char *result = NULL;
//   vector_t *v = NULL;
//   assert ((num_els && el_size) || (path && ext));
//   if (path) { // vector will be in a memory-mapped file
//     int fd = safe_open (path, ext, O_RDWR);
//     int size = safe_lseek (fd, 0, SEEK_END);
//     if (num_els) safe_truncate (fd, size = sizeof (vector_t)); 
//     v = safe_mmap (fd, 0, size, PROT_READ | PROT_WRITE, MAP_SHARED);
//     v->file = fd;
//   }
//   else v = calloc (1, sizeof (vector_t)); // vector resides in memory
//   assert (v);
//   if (num_els) { // new vector
//     v->alloc = 0;
//     v->count = 0;
//     v->esize = el_size; 
//     result = vresize (v->data, num_els); 
//     memset (result, 0, num_els * el_size); 
//   } else result = v->data;
//   return result;
// }
  
// void *vnew (unsigned num_els, unsigned el_size) {
//   void *result;
//   vector_t *v = calloc (1, sizeof (vector_t));
//   //assert (v && (num_els < vMaxCount) && (el_size < vMaxEsize));
//   assert (v);
//   v->esize = el_size;
//   result = vresize (v->data, num_els);
//   memset (result, 0, num_els * el_size); // zero out contents
//   return result;
// }

// void *vmmap (int fd, void *base_mmap) {
//   unsigned start = align_file_offset (fd);
//   void *d = base_mmap + start + sizeof (vector_t);
//   lseek (fd, vsizeof (d), SEEK_CUR);
//   return d;
// }

// void *vmmap (char *path, char *ext, unsigned num_els, unsigned el_size) {
//   vector_t *v;
//   int fd = safe_open (path, ext, O_RDWR);
//   unsigned size = lseek (fd, 0, SEEK_END);
//   if (size == 0) size = sizeof (vector_t) + nume_els * el_size;
//   safe_truncate (fd, size);
//   v = safe_mmap (fd, 0, size, PROT_READ | PROT_WRITE, MAP_SHARED);
  
  
// }

//  int main (int argc, char *argv[]) {
//    int fd;
//    char *buf;
//    unsigned  i,n, *a;
//    assert (argc == 2);
//    n = atoi (argv[1]);

//    if (n > 1) {
//      n *= 1000;
//      a = (unsigned *) vnew (10, sizeof (unsigned int));
//      fd = safe_open ("try", ".vec", O_WRONLY | O_CREAT | O_TRUNC);
//      for (vcount (a) = 0; vcount (a) < n; vcount (a) += 100) {
//        a = vcheck (a, vcount (a));
//        a [vcount(a)] = vcount(a);
//        if (vcount(a) % 1000000 == 0) {
//  	printf ("%d done\n", vcount(a));
//  	fflush (stdout);
//        }
//      }
//      i = vwrite (a, fd);
//      close (fd);
//      printf ("alloc: %u, count: %u, esize: %u, sizeof: %u\n",
//  	    vtotal(a), vcount(a), vesize(a), (unsigned) vsizeof (a));
//      fflush (stdout);
//      vfree (a);
//    }

//    else if (n > 0) {
//      fd = safe_open ("try", ".vec", O_RDONLY);
//      a = vread (fd);
//      for (i = 0; i < vcount(a); i += 100) 
//        assert (a [i] == i);
//      vfree (a);
//      close (fd);
//      printf ("done 2\n"); fflush (stdout);
//    }

//    else {
//      fd = safe_open ("try", ".vec", O_RDONLY);
//      buf = safe_mmap (fd, 0, safe_lseek (fd, 0, SEEK_END), PROT_READ, MAP_SHARED);
//      safe_lseek (fd, 0, SEEK_SET);
//      a = vmmap (fd, buf);
//      for (i = 0; i < vcount(a); i += 100) 
//        assert (a [i] == i);
//      munmap (buf, safe_lseek (fd, 0, SEEK_END));
//      close (fd);
//      printf ("done 3\n"); fflush (stdout);
//    }

//    return 0;
//  }

// *** NOTE: this does not work for some reason. assignment segfaults.
//int vector_caught_segfault;
//void vector_catch_segfault () { vector_caught_segfault = 1;}
//int vector_is_writable (vector_t *v) {
  // vector_caught_segfault = 0;
  // signal (SIGSEGV, vector_catch_segfault); // trap segmentation faults
  // v->file = v->file; // will generate segfault if read_only
  // signal (SIGSEGV, SIG_DFL); // restore segmentation faults
  // return vector_caught_segfault;
//  return (v->file != -2);
//}

/*
inline void *vcheck (void *d, id_t min_size) {
  return (min_size < vtotal(d)) ? d : resize_vec (d, 2 * min_size);
}
*/

/*
inline void *vref (void **d, unsigned index) {
  *d = vcheck (*d, index);
  if (index > len (*d)) len (*d) = index;
  return *d + vesize (*d) * (index - 1);
}
*/

/*
void *slice_vec (void *d, int beg, int end) {
  vstruct_t *v = vstruct(d);
  int sz = v->esize, N = v->count;
  if (beg < 0) beg = 0;
  if (end > N) end = N;
  unsigned n = (end - beg);
  void *result = new_vec (n, sz, 0);
  memcpy (result, d + beg*sz, n*sz);
  return result;
}
*/

/*
void append_vec (void **v1, void *v2) {
  assert (vesize (*v1) == vesize (v2));
  *v1 = vcheck (*v1, len(*v1) + len(v2));
  memcpy (*v1 + len(*v1) * vesize (*v1), 
	  v2, len(v2) * vesize (v2));
  len(*v1) += len(v2);
}
*/

/*
unsigned align_file_offset (int fd) {
  unsigned offset;
  assert (fd >= 0);
  offset = safe_lseek (fd, 0, SEEK_CUR);
  if (((offset >> 3) << 3) != offset) { // offset does not align on 64-bit
    offset = ((offset >> 3) + 1) << 3;  // set it to the next 64-bit bound
    lseek (fd, offset, SEEK_SET);
  }
  return offset;
}

unsigned vwriteF (void *d, int fd) {
  vstruct_t *v = vstruct (d);
  unsigned start = align_file_offset (fd);
  if (v->file != 1) { // need to reset for read_only mmaps
    int file = v->file; // save real descriptor 
    v->file = 1; // proper value for subsequent read_only mmaps
    write (fd, v, sizeof (vstruct_t)); // save vector_t struct
    v->file = file; // restore real descriptor
  }
  else write (fd, v, sizeof (vstruct_t));
  write (fd, d, vesize (d) * vtotal (d));
  assert ((unsigned) safe_lseek (fd, 0, SEEK_CUR) 
	  == start + vsizeof (d));
  return start + sizeof (vstruct_t);
}

void *vreadF (int fd) {
  void *d = new_vec (0, 0, 0);
  // aligning only matters for mmaped files
  // align_file_offset (fd);
  read (fd, vstruct (d), sizeof (vstruct_t));
  vstruct(d) -> file = -1; // d is in memory
  d = resize_vec (d, vtotal (d));
  read (fd, d, vesize (d) * vtotal (d));
  return d;
}
*/

//inline void *vnext (void **d) { // append one element to vector
//  *d = grow_vec (*d, len(*d) + 1);
//  return *d + vstruct(*d)->esize * len(*d)++;
//}

/*
void *new_vec (unsigned num, unsigned esize, char *path) {
  vstruct_t *v = 0; int fd = 0;   
  off_t size = sizeof(vstruct_t) + num * esize;
  if (path) {
    fd = safe_open (path, 'w');
    safe_truncate (fd, size);
    v = safe_mmap (fd, 0, size, 'w');
  }
  else v = safe_malloc (size);
  v->count = num;
  v->alloc = 0;
  v->esize = esize;
  v->file = fd;
  memset (v->data, 0, n * s);
  return v->data;
} 
void *open_vec (char *path, char access) {
  assert (access != 'w');
  int fd = safe_open (path, access);
  off_t size = safe_lseek (fd, 0, SEEK_END);
  vstruct_t *v = safe_mmap (fd, 0, size, access);
  assert (v && (size == (off_t) vsizeof (v)));
  if (v->file != 1) fprintf (stderr, "WARNING: %s may be CORRUPTED\n", path);
  if (access == 'a') v->file = fd; // will need the real descriptor
  else close (fd); // we can release fd and still keep the mmap
  return v->data;
}
*/
