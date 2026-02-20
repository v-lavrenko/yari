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

#include <math.h>
#include "mmap.h"
#include <pthread.h>

ulong myrand (ulong prev) { return prev * 1103515245 + 12345; }

// no longer needed on MacOS
//#ifdef __MACHxxx__
//#define CLOCK_MONOTONIC 1
//#include <sys/time.h>
//void clock_gettime(int tmp, struct timespec* t) {
//  struct timeval T = {0,0}; (void) tmp;
//  gettimeofday(&T, NULL);
//  t->tv_sec  = T.tv_sec;
//  t->tv_nsec = T.tv_usec * 1000;
//}
//#endif

double mstime() { // time in milliseconds
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

ulong ra_pread (int fd, off_t flen, uint size, uint n, off_t offs) {
  ulong sum = 0;
  while (--n > 0) {
    offs = myrand(offs) % (flen - size);
    char *chunk = malloc (size), *p;
    safe_pread (fd, chunk, size, offs);
    for (p = chunk; p < chunk + size; ++p) sum += *p;
    free (chunk);
  }
  return sum;
}

ulong ra_mmap (int fd, off_t flen, uint size, uint n, off_t offs) {
  ulong sum = 0;
  while (--n > 0) {
    offs = myrand(offs) % (flen - size);
    off_t beg = page_align (offs, '<'), end = page_align (offs+size, '>');
    char *ptr = safe_mmap (fd, beg, end-beg, "r");
    char *chunk = ptr + (offs - beg), *p;
    for (p = chunk; p < chunk + size; ++p) sum += *p;
    munmap (ptr, end-beg);
  }
  return sum;
}

ulong ra_MMAP (int fd, off_t flen, uint size, uint n, off_t offs) {
  ulong sum = 0;
  char *MAP = safe_mmap (fd, 0, page_align (flen, '>'), "r");
  while (--n > 0) {
    offs = myrand(offs) % (flen - size);
    char *chunk = MAP + offs, *p;
    for (p = chunk; p < chunk + size; ++p) sum += *p;
  }
  munmap (MAP, page_align (flen, '>'));
  return sum;
}

void *test_random_access (void *prm) {
  char *path = getprms (prm,"path=","RND",",");
  char *_mmap = strstr (prm,"mmap");
  char *__MMAP = strstr (prm,"MMAP");
  char *_read = strstr (prm,"read");
  //char *uring = strstr (prm,"uring");
  uint size = getprm (prm, "size=", 10000), n = getprm (prm, "n=", 10000);
  int fd = safe_open (path, "r");
  off_t flen = safe_lseek (fd, 0, SEEK_END);
  uint thread = ++threads;
  off_t offs = getprm (prm, "seed=", clock()) + thread;
  ulong sum = 0;
  double start = mstime();
  if     (__MMAP) sum = ra_MMAP  (fd, flen, size, n, offs);
  else if (_mmap) sum = ra_mmap  (fd, flen, size, n, offs);
  else if (_read) sum = ra_pread (fd, flen, size, n, offs);
  //else if (uring) sum = ra_uring (fd, flen, size, n, offs);
  double lag = mstime() - start;
  printf ("%.0fms %lx %s tread:%d\n", lag, sum, (char*)prm, thread);
  fflush(stdout);
  close (fd);
  return NULL;
}

int main (int argc, char *argv[]) {
  if (argc < 2) {
    printf ("testmmap [path=F,mmap/MMAP/read,n=1000,size=1000,seed=1]\n");
    return 1;
  }
  uint i, n = 10;
  pthread_t t[10];
  for (i = 0; i < n; i++)
    pthread_create (&t[i], NULL, test_random_access, strdup(argv[1]));
  for (i = 0; i < n; i++)
    pthread_join (t[i], NULL);
  return 0;
}
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

/* Random access using io_uring for asynchronous I/O performance */
/* This is slightly slower than ra_pread in benchmarks.
#include "liburing.h"
#include "io_uring.h"
ulong ra_uring (int fd, off_t flen, uint size, uint n, off_t seed_offset) {
  struct io_uring ring;
  const uint ring_depth = 64; // Max concurrent requests
  uint submitted = 0;
  uint completed = 0;
  off_t current_offset = seed_offset;
  ulong total_sum = 0;

  if (n-- <= 1) return 0;
  
  // Initialize io_uring with the specified depth
  if (io_uring_queue_init(ring_depth, &ring, 0) < 0) {
    perror("io_uring_queue_init");
    return 0;
  }

  // Allocate contiguous buffers for all concurrent requests
  char *io_buffers = malloc(size * ring_depth);
  if (!io_buffers) {
    io_uring_queue_exit(&ring);
    return 0;
  }

  while (completed < n) {
    // Fill the submission queue as much as possible up to ring_depth
    while (submitted < n && (submitted - completed) < ring_depth) {
      struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
      if (!sqe) break;

      current_offset = myrand(current_offset) % (flen - size);
      uint buffer_index = submitted % ring_depth;
      char *target_buffer = io_buffers + (buffer_index * size);

      // Prepare an asynchronous read operation using the prep helper
      io_uring_prep_read(sqe, fd, target_buffer, size, current_offset);
      sqe->user_data = buffer_index; // Track buffer in CQE

      submitted++;
    }

    // Submit pending requests and wait for at least one completion
    if (submitted > completed) {
      io_uring_submit(&ring);

      struct io_uring_cqe *cqe;
      if (io_uring_wait_cqe(&ring, &cqe) == 0) {
        uint buffer_index = (uint)cqe->user_data;
        char *data_ptr = io_buffers + (buffer_index * size), *p;
        
        // Sum bytes in the filled buffer
        for (p = data_ptr; p < data_ptr + size; ++p) {
          total_sum += *p;
        }

        // Retire the completion entry
        io_uring_cqe_seen(&ring, cqe);
        completed++;
      }
    }
  }

  free(io_buffers);
  io_uring_queue_exit(&ring);
  return total_sum;
}
*/
