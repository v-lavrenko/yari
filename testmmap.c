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

#include <math.h>
#include "mmap.h"

void test_random_access (char *path, char *prm) {
  char *_mmap = strstr (prm,"mmap");
  char *_read = strstr (prm,"read");
  char *pre = strstr (prm,"pre");
  uint seed = getprm (prm, "seed=", 1);
  uint size = getprm (prm, "size=", 10000);
  uint n    = getprm (prm, "n=", 10000);
  int fd = safe_open (path, "r");
  off_t flen = safe_lseek (fd, 0, SEEK_END);
  char *prealloc = malloc (size);
  ulong sum = 0;
  vtime();
  srand(seed);
  while (--n > 0) {
    off_t offs = rand() % (flen - size);
    if (_mmap) {
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
  printf ("%.2f %lx %s\n", vtime(), sum, prm);
  free (prealloc);
  close (fd);
}


int main (int argc, char *argv[]) {
  if (argc > 2) test_random_access (argv[1], argv[2]);
  else printf ("usage: %s path [mmap/read,prealloc,n=1000,size=1000,seed=1]\n", argv[0]);
  return 0;
  
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
}
