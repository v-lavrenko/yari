#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define arg(i) ((i < argc) ? argv[i] : "")

double getprm (char *params, char *name, double def) {
  if (!(params && name)) return def;
  unsigned n = strlen (name);
  char *p = strstr (params, name);
  return p ? atof (p + n) : def;
}

int main (int argc, char *argv[]) {
  char *BUF = NULL;
  size_t sz = 0;
  double step = atof(arg(1));
  if (!step) step = getprm(arg(1),"p=",2);
  unsigned int seed = getprm(arg(1),"s=",time(0));
  unsigned int thresh = RAND_MAX; //1<<31;
  unsigned int N = getprm(arg(1),"n=",0), n=0;
  srandom(seed);
  while (!feof(stdin)) {
    getline(&BUF, &sz, stdin);
    if (random() < thresh) {
      fputs(BUF,stdout);
      fflush(stdout);
      thresh /= step;
    }
    if (N && ++n > N) break;
  }
  fputs(BUF,stdout); // always print last line
}
