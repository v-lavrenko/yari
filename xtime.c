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
