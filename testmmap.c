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

int main (int argc, char *argv[]) {
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
