#include <math.h>
#include "mmap.h"

double Harmonic (double n) { // 1 + 1/2 + 1/3 + ... + 1/n
  return EulerGamma + log(n) + 0.5/n - 1./(12*n*n) + 1./(120*n*n*n*n);
}

uint *random_ranks (uint n, uint N) {
  static char *seen = NULL;
  if (!seen) seen = calloc(N,sizeof(char));
  uint *ranks = calloc(n,sizeof(uint)), i, r=0;
  for (i=0; i<n; ++i) {
    do { r = random() % N; } while (seen[r]);
    seen[r] = 1;
    ranks[i] = r;
  }
  for (i=0; i<n; ++i) seen[ranks[i]] = 0;
  return ranks;
}

float random_mrr2 (uint n, uint N) { // MRR of n ranks in [1..N] no collisions
  uint *ranks = random_ranks (n,N), i;
  float MRR = 0; 
  for (i=0; i<n; ++i) MRR += 1.0 / (1 + ranks[i]);
  free (ranks);
  return 100 * MRR / Harmonic(n);
}

float random_mrr (uint n, uint N) { // MRR of n ranks in [1..N]
  float MRR = 0; uint i = 0;
  while (++i <= n) MRR += 1.0 / (1 + random() % N);
  return 100 * MRR / Harmonic(n);
}

float best_random_mrr (uint n, uint N, uint tries) {
  uint i; float best = 0;
  for (i = 0; i < tries; ++i) {
    float MRR = random_mrr2(n,N);
    if (MRR > best) best = MRR;
  }
  return best;
}

int cmpX (const void *n1, const void *n2) {
  float *f1 = (float*)n1, *f2 = (float *)n2;
  return (*f1 < *f2) ? -1 : (*f1 > *f2) ? +1 : 0;
}

int show_distr(float *X, uint n, char *fmt) {
  uint i;
  float p[13] = {0.0001, 0.001, 0.01, 0.05, 0.1, 0.25, 0.5, 0.75, 0.9, 0.95, 0.99, 0.999, 0.9999};
  if (X) {
    qsort (X, n, sizeof(float), cmpX);
    for (i=0; i<13; ++i) printf (fmt, X[(uint)(n*p[i])]);
  } else {
    for (i=0; i<13; ++i) printf ("\t%g", p[i]);
  }
  printf ("\n");
  return 0;
}

int do_mrr(char *prm) {
  show_distr(0,0,0);
  uint iter = getprm(prm,"iter=",1000), i = 0;
  //uint n[3] = {250,300,120}, j=0;
  //uint n[23] = {1,2,3,4,5,6,7,8,9,10,15,20,25,30,40,50,60,70,80,90,100,150,200}, j=0;
  //uint n[10] = {150,200,300,500,700,1000,1500,2000,3000,5000}, j=0;
  //uint n[16] = {400,600,800,900,1250,1750,2250,2500,2750,3250,3500,3750,4000,4250,4500,4750}, j=0;
  //uint n[15] = {110,130,140,160,170,180,190,210,220,230,240,260,270,280,290}, j=0;
  uint n[10] = {310,320,330,340,350,360,370,380,390,400}, j=0;
  //uint n[24] = {1125,1375,1625,1875,2125,2375,2625,2875,3125,3375,3625,3875,4125,4375,4625,4875,5125,5250,5375,5500,5625,5750,5875,6000}, j=0;
  uint N = getprm(prm,"N=",1000);
  uint M = getprm(prm,"M=",10000);
  float *X = calloc(iter,sizeof(float));
  for (j = 0; j < 10; ++j) {
    for (i = 0; i < iter; ++i) X[i] = best_random_mrr (n[j], N, M);
    printf("%d",n[j]);
    show_distr(X,iter,"\t%.0f");
    fflush(stdout);
  }
  return 0;
}

char *_usage =
  "pval mrr,n=10,N=1000,M=10000,iter=1000\n"
  ;

int usage() { printf ("%s", _usage); return 1; }

#define a(i) ((i < argc) ? argv[i] : "")

int main (int argc, char *argv[]) {
  if (argc < 2) return usage();
  uint seed = getprm(a(1),"seed=",0);
  if (seed) srandom(seed);
  if (!strncmp(a(1),"hdr",3)) return show_distr(0,0,0);
  if (!strncmp(a(1),"mrr",3)) return do_mrr(a(1));
  return usage();
}
