#include <math.h>
#include "matrix.h"
#include "hash.h"
#include "textutil.h"

#define beg_of_day(P,t)   ((t==0) || (P[t].i - P[t-1].i > 7*3600))
#define end_of_day(P,t,n) ((t==n) || (P[t+1].i - P[t].i > 7*3600))

static float bp(ix_t *in, ix_t *out) { return 10000 * (out->x / in->x - 1) }

ix_t *ts_close2open(ix_t *P) {
  ix_t *out = new_vec(0,sizeof(ix_t)), new = {0,0}, *p;
  for (p = P; p < P+len(P); ++p) {
    
  }
  while (++p < endP) {
    if (p->i - q->i > 28800) {
      ix_t new = {p->i, bp(q,p)};
      out = append_vec (out, &new);
    }
    q = P;
  }
  uint t, n = len(P);
  for (t = 1; t < n; ++t) {
    if (beg_of_day(P,t)) {};
    if (end_of_day(P,t,n-1)) {};
  }
  return;
}
