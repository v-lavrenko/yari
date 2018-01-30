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

void safe_kstem (char *word) ;
void kstem_stemmer (char *word, char *stem) ;

void lowercase_stemmer (char *string, char *result) {
  char *s;
  strncpy (result, string, 999);
  for (s = result; *s; ++s) 
    *s = tolower ((int) *s);
}

void stem_rcv (char *prm) {
  char *r = strstr(prm,"row"), *c = strstr(prm,"col");
  void (*stem) (char*,char*) = NULL; 
  if (strstr (prm, "lower" )) stem = lowercase_stemmer;
  else                        stem = kstem_stemmer;
  float value;
  char line[1000], row[1000], col[1000], rstem[1000], cstem[1000];
  while (fgets (line, 999, stdin)) {
    if (*line == '#') {printf ("%s", line); continue; }// skip comments
    if (3 != sscanf (line, "%s %s %f", row, col, &value)) {
      fprintf (stderr, "cannot parse: %s", line); continue; }
    if (r) stem (row, rstem);
    if (c) stem (col, cstem);
    printf ("%15s %15s %10.4f\n", (r?rstem:row), (c?cstem:col), value);
  }    
}

char *usage = "stem [krovetz,lower],[row] < rcv > stemmed\nstem -test f1 f2 f3 ...\n";

void *test_kstem_thread (void *_in) { // thread-safe
  static int threads = 0;
  int this = ++threads, lines = 0;
  char *nl, *in = _in, out[1000], line[999], stem[999];
  sprintf (out,"%s.out",in);
  fprintf (stderr,"thread %d: %s -> %s\n", this, in, out);
  FILE *IN  = fopen (in,"r");
  FILE *OUT = fopen (out,"w");
  while (fgets (line,999,IN)) {
    if ((nl = strchr(line,'\n'))) *nl = '\0'; // strip newline
    kstem_stemmer (line, stem);
    fputs (stem, OUT); 
    fputc ('\n', OUT);
    ++lines;
  }
  fclose(IN); fclose(OUT); 
  fprintf (stderr,"thread %d: %d lines done\n", this, lines);
  return NULL;
}

void *detach (void *(*handle) (void *), void *arg) ;

int main (int argc, char *argv[]) {
  if (argc > 1 && !strncmp(argv[1],"-h",2)) { printf ("%s", usage); return 1; }
  char *bin = getenv ("STEM_DIR");
  if (!bin) {
    fprintf (stderr, "*** $STEM_DIR environment variable must be set\n");
    return 1;
  }
  
  if (argc > 2 && !strcmp(argv[1],"-test")) {
    while (--argc > 2) detach (test_kstem_thread, argv[argc]);
    test_kstem_thread (argv[2]);
    sleep(10);
    return 0;
  }
  
  stem_rcv (argc > 1 ? argv[1] : "");
  return 0;
}
