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

void kstem_stemmer (char *word, char *stem) ;

void lowercase_stemmer (char *string, char *result) {
  char *s;
  strncpy (result, string, 999);
  for (s = result; *s; ++s) 
    *s = tolower ((int) *s);
}

void stem_rcv (char *prm) {
  char *r = strstr(prm,"row");
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
    if (1) stem (col, cstem);
    printf ("%15s %15s %10.4f\n", (r?rstem:row), cstem, value);
  }    
}

char *usage = "stem [krovetz,lower],[row] < rcv > stemmed\n";

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
