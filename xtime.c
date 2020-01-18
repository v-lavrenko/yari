//#include "types.h"
//#include "mmap.h"
#include "vector.h"
#include "hash.h"
#include "timeutil.h"
#include "textutil.h"

int main (int n, char *A[]) {
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
