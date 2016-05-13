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

#include <math.h>
#include "mmap.h"
#include <pthread.h>

ulong myrand (ulong prev) { return prev * 1103515245 + 12345; }

#ifdef __MACH__
#define CLOCK_MONOTONIC 1
#include <sys/time.h>
void clock_gettime(int tmp, struct timespec* t) {
  struct timeval T = {0,0}; (void) tmp;
  gettimeofday(&T, NULL);
  t->tv_sec  = T.tv_sec;
  t->tv_nsec = T.tv_usec * 1000;
}
#endif

double mstime() {
  struct timespec tp;
  clock_gettime (CLOCK_MONOTONIC, &tp);
  return 1000.*tp.tv_sec + tp.tv_nsec/1E6;
}

void detach (void *(*handle) (void *), void *arg) {
  pthread_t t = 0;
  if (pthread_create (&t, NULL, handle, arg)) assert (0);
  if (pthread_detach (t)) assert (0);
}

uint threads = 0;
void *test_random_access (void *prm) {
  char *path = getprms (prm,"path=","RND",',');
  char *_mmap = strstr (prm,"mmap");
  char *_read = strstr (prm,"read");
  char *__MMAP = strstr (prm,"MMAP");
  char *pre = strstr (prm,"pre");
  uint size = getprm (prm, "size=", 10000);
  uint n    = getprm (prm, "n=", 10000);
  int fd = safe_open (path, "r");
  off_t flen = safe_lseek (fd, 0, SEEK_END);
  uint thread = ++threads;
  off_t offs = getprm (prm, "seed=", clock()) + thread;
  char *prealloc = malloc (size), *MAP = NULL;
  ulong sum = 0;
  double start = mstime();
  while (--n > 0) {
    offs = myrand(offs) % (flen - size);
    if (__MMAP) { // entire file memory-mapped
      if (!MAP) MAP = safe_mmap (fd, 0, page_align (flen, '>'), "r");
      char *chunk = MAP + offs, *p;
      for (p = chunk; p < chunk + size; ++p) sum += *p;
    } else if (_mmap) {
      off_t beg = page_align (offs, '<'), end = page_align (offs+size, '>');
      char *ptr = safe_mmap (fd, beg, end-beg, "r");
      char *chunk = ptr + (offs - beg), *p;
      for (p = chunk; p < chunk + size; ++p) sum += *p;
      munmap (chunk, end-beg);
    } else if (_read) {
      char *chunk = pre ? prealloc : malloc (size), *p;
      safe_pread (fd, chunk, size, offs);
      for (p = chunk; p < chunk + size; ++p) sum += *p;
      if (!pre) free (chunk);
    } 
  }
  if (MAP) munmap (MAP, page_align (flen, '>'));
  double lag = (mstime() - start);
  fprintf (stderr, "%.0fms %lx %s tread:%d\n", lag, sum, (char*)prm, thread);
  free (prealloc); 
  close (fd);
  return NULL;
}

int main (int argc, char *argv[]) {
  if (argc > 1) {
    //pthread_t t = 1;
    //if (pthread_create (&t, NULL, test_random_access, argv[1])) assert (0);
    //if (pthread_detach (t)) assert (0);
    uint n = 10;
    while (--n > 0) detach (test_random_access, (void*)strdup(argv[1]));
    test_random_access (argv[1]);
    sleep(10);
  }
  else printf ("usage: %s [path=F,mmap/read,prealloc,n=1000,size=1000,seed=1]\n", argv[0]);
  return 0;

/*
  if (argc < 2) { fprintf (stderr, "Usage: %s n\n", *argv); return -1;}
  ulong N = (1ul << atoi (argv[1])), M = 1<<28, sz = sizeof(ulong), iter = 0;
  char *NAME = (argc > 2) ? argv[2] : "trymmap.mmap";
  
  srandom(1);
  mmap_t *m = open_mmap (NAME, "w", M * sz); 
  expand_mmap (m, N * sz);
  free_mmap(m);
  while (1) {
    m = open_mmap (NAME, "a", M * sz); 
    for (iter = 0; iter < 100; ++iter) {
      ulong beg = random()%(N-M), num = random()%M, i, nz=0;
      ulong *x = move_mmap (m, (off_t)beg*sz, (off_t)num*sz);
      for (i = 0; i < num; ++i) 
	if (x[i]) {assert (x[i] == beg+i); ++nz;}
	else x[i] = beg+i;
      fprintf (stderr, "[%.0fs] filled %lu + %lu: %lu matched\n", 
	       vtime(), beg, num, nz); 
    }
    free_mmap(m);
    fprintf(stderr, "-------------\n");
  }
  
  return 1;
*/
}
