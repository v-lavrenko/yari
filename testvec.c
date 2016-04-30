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
#include "vector.h"

void test_resize() {
  void *v = open_vec ("XXX", "w", 4);
  v = resize_vec (v,  536870911);
  v = resize_vec (v, 1073741823);
  v = resize_vec (v, 2147483647);
  v = resize_vec (v, 4294967295);
  free_vec (v);
}

int main (int argc, char *argv[]) {
  unsigned i, n, j = 1;
  unsigned *a;
  //test_resize(); return 0;
  if (argc > 2 && !strcmp(argv[1],"reset")) {
    fprintf (stderr, "re-setting %s: ", argv[2]);
    free_vec (open_vec (argv[2], "a", 0));
    fprintf (stderr, "done\n");
  }
  return 0;
    
  char *FILE_NAME = "tryvec.mmap";
  if (argc < 2) { fprintf (stderr, "Usage: %s n\n", *argv); return -1;}
  n = 1 << atoi (argv[1]);
  if (argc > 2) FILE_NAME = argv[2];
  
  /*
  double s = 0;
  for (i=0; i<n; ++i){
    s += ilog2(i);
    //printf ("%10d %10d %10d %10d %10.0f\n", i, next_pow2(i), 
    //ilog2(i), fast_log2(i), (float)log2l((long double)i));
  }
  printf ("sum=%f\n", s);
  return 0;
  */
  
  /*{  // win32 does not support rlimit routines
    struct rlimit lim;
    getrlimit (RLIMIT_DATA, &lim);
    printf ("Memory limits: %lu %lu\n", lim.rlim_cur, lim.rlim_max);
    getrlimit (RLIMIT_VMEM, &lim);
    printf ("Address limits: %lu %lu\n", lim.rlim_cur, lim.rlim_max);
    }*/
  
  vtime();
  printf ("mmap(w) -> resize  -> free ");
  for (j = 0; j < 10; ++j) {
    a = open_vec (FILE_NAME, "w", sizeof(unsigned));
    for (i = 0; i < n; ++i) {
      a = resize_vec (a, i+1);
      a[i] = i; 
    }
    free_vec (a);
    printf (" %.2f", vtime()); fflush(stdout);
  }
  printf ("\n");
  
  printf ("mmap(r) -> check -> free ");
  for (j = 0; j < 10; ++j) {
    a = open_vec (FILE_NAME, "r", 0);
    assert (len(a) == n && vesize(a) == sizeof(unsigned));
    for (i = 0; i < n; ++i) if (a[i] != i) assert(0);
    free_vec (a);
    printf (" %.2f", vtime()); fflush(stdout);
  }
  printf ("\n");

  vtime();
  printf ("mmap(w) -> append  -> free ");
  for (j = 0; j < 10; ++j) {
    a = open_vec (FILE_NAME, "w", sizeof(unsigned));
    for (i = 0; i < n; ++i) {
      a = append_vec (a, &i);
    }
    free_vec (a);
    printf (" %.2f", vtime()); fflush(stdout);
  }
  printf ("\n");

  printf ("mmap(w) -> check -> free ");
  for (j = 0; j < 10; ++j) {
    a = open_vec (FILE_NAME, "a", 0);
    assert (len(a) == n && vesize(a) == sizeof(unsigned));
    for (i = 0; i < n; ++i) if (a[i] != i) assert(0);
    free_vec (a);
    printf (" %.2f", vtime()); fflush(stdout);
  }
  printf ("\n");

  printf ("malloc -> append -> write");
  for (j = 0; j < 10; ++j) {
    a = new_vec (0, sizeof (unsigned));
    for (i = 0; i < n; ++i) {
      a = append_vec (a, &i);
      //a = resize_vec (a, i+1);
      //a[i] = i; 
    }
    write_vec (a, FILE_NAME);
    free_vec (a);
    printf (" %.2f", vtime()); fflush(stdout);
  }
  printf ("\n");
  
  printf ("read    -> check -> free ");
  for (j = 0; j < 10; ++j) {
    a = read_vec (FILE_NAME);
    assert (len(a) == n && vesize(a) == sizeof(unsigned));
  for (i = 0; i < n; ++i) if (a[i] != i) assert(0);
  free_vec (a);
  printf (" %.2f", vtime()); fflush(stdout);
  }
  printf ("\n");
  
  /*  
  
  */
  
/*   for (i = 0; i < n; ++i) { */
/*     a = vmmap (FILE_NAME, write_shared); */
/*     vfree (a); */
/*   } */
/*   printf ("[%f] repeatedly mapped and freed write_shared\n", vtime()); */

/*   for (i = 0; i < n; ++i) { */
/*     a = vmmap (FILE_NAME, read_only); */
/*     vfree (a); */
/*   } */
/*   printf ("[%f] repeatedly mapped and freed read_only\n", vtime()); */

/*   for (i = 0; i < n; ++i) { */
/*     a = vnew (n, sizeof(unsigned), 0); */
/*     vfree (a); */
/*   } */
/*   printf ("[%f] repeatedly allocced and freed \n", vtime()); */
  
  return 0;
}

