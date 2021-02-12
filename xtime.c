/*
  
  Copyright (c) 1997-2021 Victor Lavrenko (v.lavrenko@gmail.com)
  
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

//#include "types.h"
//#include "mmap.h"
#include "vector.h"
#include "hash.h"
#include "timeutil.h"
#include "textutil.h"

int benchmark_time() {
  uint n; double t0, t;
  t = t0 = ftime();
  for (n = 0; t - t0 < 5; ++n) t = ftime();
  fprintf(stderr,"%d ftime() in %.3fs\n", n, t-t0);
  t = t0 = mstime();
  for (n = 0; t - t0 < 5000; ++n) t = mstime();
  fprintf(stderr,"%d mstime() in %.3fs\n", n, (t-t0)/1E3);
  t = t0 = time(0);
  for (n = 0; t - t0 < 5; ++n) t = time(0);
  fprintf(stderr,"%d time() in %.3fs\n", n, t-t0);  
  ulong T0 = ustime(), T = T0;
  for (n = 0; T - T0 < 5E6; ++n) T = ustime();
  fprintf(stderr,"%d ustime() in %ldus\n", n, T-T0);  
  return 0;
}

int main (int n, char *A[]) {
  if (n>1 && !strcmp(A[1], "-bench")) return benchmark_time();	   
  char *_col = "1", *ifmt = "%s", *ofmt = "%F,%T";  int i;
  for (i=1; i<n; ++i) {    
    if (!strncmp(A[i],"-f",2)) _col = A[i][2] ? A[i]+2 : ++i<n ? A[i] : _col;
    if (!strncmp(A[i],"-i",2)) ifmt = A[i][2] ? A[i]+2 : ++i<n ? A[i] : ifmt;
    if (!strncmp(A[i],"-o",2)) ofmt = A[i][2] ? A[i]+2 : ++i<n ? A[i] : ofmt;
  }
  uint col = atoi(_col), c;
  if (n < 2 || col < 1 || !index(ifmt,'%') || !index(ofmt,'%')) {
    fprintf(stderr, "USAGE: xtime -f 1 -i '%%s' -o '%%F'\n");
    return 1;
  }
  --col;
  char line[9999], buf[999];
  while (fgets (line, 9998, stdin)) {
    noeol(line);
    char **F = split(line,'\t');
    time_t t = strf2time (F[col], ifmt);
    F[col] = time2strf (buf, ofmt, t);
    //printf("%ld %s ", t, buf);
    for (c=0; c<len(F); ++c) { if (c) fputc('\t',stdout); fputs(F[c],stdout); }
    fputc('\n', stdout);
  }
  return 0;
}
