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

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/errno.h>
#include <sys/fcntl.h>

void porter_stemmer (char *word, char *stem) ;
void kstem_stemmer (char *word, char *stem) ;
void arabic_stemmer (char *word, char *stem) ;

void lowercase_stemmer (char *string, char *result) {
  char *s;
  strncpy (result, string, 999);
  for (s = result; *s; ++s) 
    *s = tolower ((int) *s);
}

void stem_rcv (char *prm) {
  char *r = strstr(prm,"row");
  void (*stem) (char*,char*) = NULL; 
  if      (strstr (prm, "porter")) stem = porter_stemmer;
  else if (strstr (prm, "lower" )) stem = lowercase_stemmer;
  else                             stem = kstem_stemmer;
  float value;
  char line[1000], row[1000], col[1000], rstem[1000], cstem[1000];
  while (fgets (line, 999, stdin)) {
    if (*line == '#') {printf ("%s", line); continue; }// skip comments
    if (3 != sscanf (line, "%s %s %f", row, col, &value)) {
      fprintf (stderr, "cannot parse: %s", line); continue; }
    if (r) stem (row, rstem);
    if (1) stem (col, cstem);
    printf ("%15s %15s %10.4f\n", (r?rstem:row), cstem, value);
  }    
}

char *usage = "stem [krovetz,porter,lower],[row] < rcv > stemmed\n";

int main (int argc, char *argv[]) {
  if (argc > 1 && !strncmp(argv[1],"-h",2)) { printf ("%s", usage); return 1; }
  char *bin = getenv ("STEM_DIR");
  if (!bin) {
    fprintf (stderr, "*** $STEM_DIR environment variable must be set\n");
    return 1;
  }
  stem_rcv (argc > 1 ? argv[1] : "");
  return 0;
}
