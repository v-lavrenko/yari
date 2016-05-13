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

#include "coll.h"

// #define foreach(M,v) 
// #define looks only for commas and parens!
//#define repeat_until(block,predicate) do {block} while (!(predicate)
//(4); repeat_until({int x,y; x=h(x,y);},z);
/*
#define foreach(M,i,v,codeblock) {					\
    unsigned i;								\
    for (i = 1; i <= m_count(M); ++i) if (_ok(M,i)) {			\
	void *v = m_get_vec (M, i, 0, 0);				\
	{codeblock}							\
      } }
*/
/*
#define vfor(V,v,codeblock) {			\
    wnode_t *v;					\
    for (v = V;v < vend(V); ++v) {codeblock} }
*/

int main (int argc, char *argv[]) {
  
  if (argc < 2) { 
    fprintf (stderr, "Usage: %s NAME rows cols\n", argv[0]); 
    fprintf (stderr, "       %s -visual MATRIX\n", argv[0]); 
    fprintf (stderr, "       %s -layout MATRIX\n", argv[0]); 
    fprintf (stderr, "       %s -defrag MATRIX RESULT\n", argv[0]); 
    return -1;
  }
  
  if (!strcmp(argv[1],"-dump")) {
    coll_t *c = open_coll (argv[2], "r+");
    uint i = 0;
    fprintf (stderr, "%s: %d vecs, flen: %ld\n", argv[2], nvecs(c), (ulong)(c->vecs->flen));
    printf ("%6d <- %6d -> %6d : %10ld\n", 
	    c->prev[i], i, c->next[i], (ulong)(c->offs[i]));
    while ((i = c->next[i])) {
      vec_t *vec = vect (get_vec (c, i));
      //printf ("%d <- %d -> %d : %ld\n", 
      //      c->prev[i], i, c->next[i], c->offs[i]);
      printf ("%6d <- %6d -> %6d : %10ld : %10ld = %2d x %6lu > %6u\n", 
	      c->prev[i], i, c->next[i], (ulong)(c->offs[i]), 
	      (ulong)(vsizeof(vec)), vec->esize, vlimit(vec), vec->count);
      free_vec (vec->data);
    }
    free_coll (c);
    return 0;
  }

  if (!strcmp(argv[1],"-dump0")) {
    coll_t *c = open_coll (argv[2], "r");
    uint i = 0;
    fprintf (stderr, "%s: %d vecs, flen: %ld\n", argv[2], nvecs(c), (ulong)(c->vecs->flen));
    for (i = 0; i <= nvecs(c); ++i) 
      printf ("%6d : %10ld\n", i, (ulong)(c->offs[i]));
    free_coll (c);
    return 0;
  }
  
  return 0;

  //char spinner[] = "|/-\\";
  char *NAME = argv[1];
  uint nr = (1<<atoi(argv[2]))-1, nni = atoi(argv[3]), r;
  ulong nR = 0, nW = 0, sz = sizeof(uint), MB = 1<<20;
  float t, t2, wait = atof(argv[4]);
  coll_t *c;
  
  srandom(1); nR = nW = 0;
  c = open_coll (NAME, "w+");
  vtime();
  for (t = 0; t < wait; t += vtime()) {
    r = 1 + random() % nr;
    uint ni = 1 << (random() % nni);
    uint *vec = has_vec(c,r) ? get_vec(c,r) : new_vec(0,sz);
    uint i = len (vec);
    vec = resize_vec (vec, i+ni);
    for (; i < len(vec); ++i) vec[i] = i;
    put_vec (c, r, vec);
    free_vec (vec);
    nW += ni;
  }
  free_coll (c);
  
  c = open_coll (NAME, "r+");
  //assert (nvecs(c) == nr);
  vtime();
  for (r = 1; r <= nr; ++r) {
    uint *vec = get_vec (c, r), i;
    if (!vec) continue;
    assert (vesize(vec) == sz);
    for (i=0; i < len(vec); ++i) assert (vec[i] == i);
    nR += len(vec);
    free_vec (vec);
  }
  t2 = vtime();
  free_coll (c);
  
  assert (nR == nW);
  
  printf("%s: %d vecs += %d els: %ld gets, %ld puts, %.0f / %.0f MB/s\n",
	 NAME, nr, nni, nW, nR, ((nW*sz) / (t*MB)), ((nR*sz) / (t2*MB)));
  
  srandom(1); nR = nW = 0;
  /*
  c = open_coll (NAME, "w+");
  vtime();
  for (t = 0; t < wait; t += vtime()) {
    r = 1 + random() % nr;
    uint ni = 1 << (random() % nni);
    uint *vec = mmap_vec (c,r,ni,sz);
    uint i = len (vec) - ni;
    for (; i < len(vec); ++i) vec[i] = i;
    nW += ni;
  }
  free_coll (c);
  
  c = open_coll (NAME, "r+");
  //assert (nvecs(c) == nr);
  vtime();
  for (r = 1; r <= nr; ++r) {
    if (!has_vec(c,r)) continue;
    uint *vec = mmap_vec (c, r, 0, 0), i;
    assert (vesize(vec) == sz);
    for (i=0; i < len(vec); ++i) assert (vec[i] == i);
    nR += len(vec);
  }
  t2 = vtime();
  free_coll (c);
  */
  
  assert (nR == nW);
  
  printf("%s: %d vecs += %d els: %ld wmap, %ld rmap, %.0f / %.0f MB/s\n",
	 NAME, nr, nni, nW, nR, ((nW*sz) / (t*MB)), ((nR*sz) / (t2*MB)));
  
  /*  
  if (!strcmp(argv[1],"-visual")) {
    m = m_open (argv[2], read_only);
    m_visualize (m);
    m_free (m);
    return 0; 
  }
  
  if (!strcmp(argv[1],"-layout")) {
    m = m_open (argv[2], read_only);
    m_layout (m);
    m_free (m);
    return 0; 
  }

  if (!strcmp(argv[1],"-defrag")) {
    m_defrag (argv[2], argv[3]); 
    return 0; 
  }
  */

  
  /*
  
  printf ("Timings for a %d x %d matrix\n", nr, nc);
  vtime();
  srandom(1);
  ops = 0;
  for (t = 0; t < wait; t += vtime()) {
    for (i = 0; i < 10; ++i) {
      unsigned r = random() % nr;
      unsigned *vec = get_vec (c,r);
      if (!vec) vec = new_vec (0, sizeof(unsigned));
      vec = resize_vec (vec, len(vec)+1);
      a = mget_vec (NAME, x, write_shared);
      a = a ? a : mnew_vec (NAME, x, 1, sizeof(unsigned));
      for (j = 0; j < 100; ++j) {
	y = (random() % Y) * (j+1) / 100;
	a = vcheck (a, y+1);
	if (y >= vcount(a)) vcount(a) = y+1;
	a[y] = x + y;
      }
      vfree (a);
    }
    ++ops;
  }
  printf ("%10.2fK writes per second (%d/%f)\n", ops/time, ops, time);


  srandom(1);
  for (time = 0; time < wait; time += vtime()) {
    
    unsigned r = random() % nr, c = random()
  
  NAME = argv[1];
  X = 
    Y = atoi (argv[3]);
  TOUT = atoi (argv[4]);
    free_matrix (m);

  if (1) {
  
  printf ("Timings for a %d x %d matrix of type 1\n", X, Y);
  vtime();
  srandom(1);
  ops = 0;
  for (time = 0; time < TOUT; time += vtime()) {
    for (i = 0; i < 10; ++i) {
      x = random() % X;
      a = mget_vec (NAME, x, write_shared);
      a = a ? a : mnew_vec (NAME, x, 1, sizeof(unsigned));
      for (j = 0; j < 100; ++j) {
	y = (random() % Y) * (j+1) / 100;
	a = vcheck (a, y+1);
	if (y >= vcount(a)) vcount(a) = y+1;
	a[y] = x + y;
      }
      vfree (a);
    }
    ++ops;
  }
  printf ("%10.2fK writes per second (%d/%f)\n", ops/time, ops, time);
  
  vtime();
  srandom(1);
  for (k = 0; k < ops; ++k) {
    for (i = 0; i < 10; ++i) {
      x = random() % X;
      a = mget_vec (NAME, x, read_only);
      assert (a);
      for (j = 0; j < 100; ++j) {
	y = (random() % Y) * (j+1) / 100;
	assert (y < vcount(a));
	assert (a[y] == x+y);
	//if (a[y] != (x + y))
	//  printf ("%d <> %d TEST[%d][%d]\n", 
	//	  x+y, a[y], x, y);
      }
      vfree (a);
    }
  }
  time = vtime();
  printf ("%10.2fK reads per second (%d/%f)\n", ops/time, ops, time);

  }
  
  ///////////////////////// TYPE 2 /////////////////////////
  
  printf ("Timings for a %d x %d matrix of type 2\n", X, Y);

  vtime();
  srandom(1);
  ops = 0;
  m = m_new (NAME, 1);
  for (time = 0; time < TOUT; time += vtime()) {
    for (i = 0; i < 10; ++i) {
      x = random() % X;
      a = m_get_vec (m, x+1, 1, sizeof(unsigned));
      for (j = 0; j < 100; ++j) {
	y = (random() % Y) * (j+1) / 100;
	a = m_chk_vec (a, m, x+1, y+1);
	if (y >= vcount(a)) vcount(a) = y+1;
	a[y] = x + y;
      }
    }
    ++ops;
  }
  m_free(m);
  printf ("%10.2fK writes per second (%d/%f)\n", ops/time, ops, time);
  
  vtime();
  srandom(1);
  m = m_open (NAME, read_only);
  for (k = 0; k < ops; ++k) {
    for (i = 0; i < 10; ++i) {
      x = random() % X;
      a = m_get_vec (m, x+1, 0, 0);
      assert(a);
      for (j = 0; j < 100; ++j) {
	y = (random() % Y) * (j+1) / 100;
	assert (y < vcount(a));
	assert (a[y] == x+y);
	//if (a[y] != (x + y))
	//  printf ("%d <> %d == TEST[%d][%d]\n", x+y, a[y], x, y);
      }
    }
  }
  time = vtime();
  m_free(m);
  printf ("%10.2fK reads per second (%d/%f)\n", ops/time, ops, time);
  */
    
  return 0;
}
