#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main (int argc, char *argv[]) {
  char *BUF = NULL;
  size_t N = 0;
  double step = (argc > 1) ? atof(argv[1]) : 2;
  unsigned int thresh = 1<<31;
  srandom(time(0));
  while (getline(&BUF, &N, stdin) > 0) 
    if (random() < thresh) {
      fputs(BUF,stdout);
      thresh /= step;
    }
}
