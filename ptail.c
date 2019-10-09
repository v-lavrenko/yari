#include <stdio.h>
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
  size_t N = 0, n = 0;
  double step = getprm(arg(1),"p=",2);
  unsigned int seed = getprm(arg(1),"s=",time(0));
  unsigned int thresh = RAND_MAX; //1<<31;
  srandom(seed);
  while (!feof(stdin)) {
    n = getline(&BUF, &N, stdin);
    if (random() < thresh) {
      fputs(BUF,stdout);
      thresh /= step;
    }
  }
  fputs(BUF,stdout); // always print last line
}
