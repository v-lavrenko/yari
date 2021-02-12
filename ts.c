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

#include <math.h>
#include "matrix.h"
#include "hash.h"
#include "textutil.h"
#include "timeutil.h"

#define OVERNIGHT 25200 // 7 hours

//#define beg_of_day(P,t)   ((t==0) || (P[t].i - P[t-1].i > OVERNIGHT))
//#define end_of_day(P,t,n) ((t==n) || (P[t+1].i - P[t].i > OVERNIGHT))

#define beg_of_day(p,P)    (((p) <= (P))    || ((p)->i - ((p)-1)->i > OVERNIGHT))
#define end_of_day(p,last) (((p) >= (last)) || (((p)+1)->i - (p)->i > OVERNIGHT))
#define gain2bp(gain) (10000 * ((gain) - 1))
#define bp2gain(bp) (1 + (bp) / 10000)

float BP(float in, float out) {return in ? (10000.0 * (out / in - 1.0)) : 0; }
float LP(float in, float out) {return (in>0 && out>0) ? log(out/in) : 0; }
float CP(float in, float out) {return round(100 * (out - in)); }

float addBP(float p, float bp) { return p * bp2gain(bp); }

ix_t *prev_open(ix_t *p, ix_t *P) {
  while (!beg_of_day(p,P)) --p;
  return MAX(p,P);
}

ix_t *next_close(ix_t *p, ix_t *P) {
  ix_t *last = P+len(P)-1;
  while (!end_of_day(p,last)) ++p;
  return MIN(p,last);
}

// -------------------- TARGETS: hi,lo,trail,wait  -------------------- 

// exit point q for a position entered at p
ix_t *find_exit (ix_t *p, ix_t *P, float _gain, float _loss, char trail, uint wait) {
  float gain = bp2gain(_gain), loss = bp2gain(_loss);
  float hi = gain * p->x, lo = loss * p->x;
  uint T = p->i + wait; // 
  ix_t *q = p-1, *last = P+len(P)-1;
  while (++q < last) {
    //if (day && end_of_day(q,last)) break;     // market-on-close
    //char *tag = (beg_of_day(q,P) ? "open" : end_of_day(q,last) ? "close" : (q->i > T) ? "wait" : ""), _[999];
    //printf("%s %.2f %s", time2str(_,q->i), q->x, tag);
    //if ((trail == '^') && (lo < loss * q->x)) printf("lo: %.2f -> %.2f\n", lo, loss*q->x);
    //if ((trail == 'v') && (hi > gain * q->x)) printf("hi: %.2f -> %.2f\n", hi, gain*q->x);
    if (q[1].i > T) break; // timeout
    if ((q->x >= hi) || (q->x <= lo)) break; // hit limit
    if (trail == '^') lo = MAX(lo,loss*q->x); // raise floor if price moved up
    if (trail == 'v') hi = MIN(hi,gain*q->x); // lower ceiling if price dropped
  }
  //printf("exit: %.2f in %.2f..%.2f\n", q->x, lo, hi);  
  return q;
}

// while (!end_of_day(q,last) && (q->x < hi) && (q->x > lo)) ++q;

int ts_targets (char *TRG, char *PRC, char *prm) {
  // char *side  = getprmp(prm,"side=","-"); 
  char *trail = getprmp(prm,"trail=","-");
  char *exits = strstr(prm,"exits"); //, *day = strstr(prm,"day");
  char *binary = strstr(prm,"binary"); // binarize to +/- 1
  uint wait = str2seconds (getprmp (prm,"wait=","7h"));
  float gain = getprm(prm,"gain=",100000);
  float loss = -getprm(prm,"loss=",10000);
  coll_t *P = open_coll (PRC, "r+"), *T = open_coll (TRG, "w+");
  uint id, n = num_rows(P);
  printf ("%s targets: wait %ds for %.0f..%.0fbp %s %s %s\n", PRC, wait, loss, gain,
	  (exits?"exits":""), (binary?"binary":""), ((*trail=='-')?"":"trailing"));
  for (id = 1; id <=n; ++id) {
    ix_t *V = get_vec (P,id), *in = V-1, *end = V+len(V);
    while (++in < end) {
      ix_t *out = find_exit (in, V, gain, loss, *trail, wait);
      if (exits) *in = *out; // replace entry with matching exit
      else in->x = BP(in->x,out->x); // replace price with entry-exit BP
      if (binary) in->x = (in->x >= gain) ? +1 : (in->x <= loss) ? -1 : 0;
    }
    put_vec (T,id,V);
    free_vec(V);
    if (!(id%10)) show_progress (id, n, "rows");
  }
  free_coll (P); free_coll (T);
  return 0;
}

// -------------------- PRICE patterns: +---+-+  -------------------- 

char *price2signs (ix_t *P) {
  char *S = calloc(len(P)+1,1);
  ix_t *p = P-1, *last = P+len(P)-1;
  while (++p <= last) S[p-P] = (beg_of_day(p,P) ? ' ' :
				p->x > (p-1)->x ? '+' :
				p->x < (p-1)->x ? '-' : '.');
  return S;
}

int ts_signs (char *PRC, char *TIC) {
  hash_t *T = open_hash (TIC, "r");
  coll_t *P = open_coll (PRC, "r+");
  uint id, n = num_rows(P);
  for (id = 1; id <=n; ++id) {
    ix_t *V = get_vec (P,id);
    char *S = price2signs(V), *tic = id2key(T,id);
    fputs (tic,stdout); fputs(S,stdout); putchar('\n');
    free(S); free_vec(V);
  }
  free_coll (P); free_hash (T);
  return 0;
}

ix_t *price2codes (ix_t *P, uint nbits, char *intraday) {
  char buf[99]; (void) buf;
  ix_t *C = copy_vec(P); C->x = 0;
  uint code = 0, mask = (1<<nbits) - 1; // 01000000 -> 00111111
  ix_t *p = P, *last = P+len(P)-1, *start = P+nbits;
  while (++p <= last) {
    uint bit = (p->x) > ((p-1)->x) ? 1 : 0;
    code = (code<<1) | bit; // shift + OR
    if (intraday && beg_of_day(p,P)) start = p+nbits;    
    if (p < start) C[p-P].x = 0; // not enough for pattern
    else C[p-P].x = 1 + (code & mask); // last nbits of code
    //printf ("%ld %s %.2f %.2f %d %u %.0f\n", p-P, time2str(buf,p->i), p->x, (p-1)->x, bit, (code&mask), C[p-P].x);
  }
  return C;
}

int ts_codes (char *COD, char *PRC, char *prm) {
  char *intraday = strstr(prm,"day");
  uint nbits = getprm(prm,"bits=",10);
  coll_t *C = open_coll(COD,"w+"), *P = open_coll (PRC, "r+");
  uint id, n = num_rows(P);
  for (id = 1; id <=n; ++id) {
    ix_t *V = get_vec (P,id);
    ix_t *c = price2codes (V, nbits, intraday);
    put_vec (C,id,c);
    free_vec (V); free_vec (c);
    if (!(id%10)) show_progress (id, n, "rows");    
  }
  free_coll (P); free_coll (C);
  return 0;
}

// ------------------------ simple signals ------------------------

static double window_avg (ix_t *q, ix_t *p) {
  double A = 0, N = MAX(1,p-q);
  while (++q <= p) A += q->x;
  return A / (N?N:1);
}

static double window_min (ix_t *q, ix_t *p) {
  double A = p->x;
  while (++q <= p) A = MIN(A,q->x);
  return A;
}

static double window_max (ix_t *q, ix_t *p) {
  double A = p->x;
  while (++q <= p) A = MAX(A,q->x);
  return A;
}

double window_var (ix_t *q, ix_t *p) {
  double N = MAX(1,p-q), SX = 0, SX2 = 0, X;
  while (++q <= p) { X = q->x; SX += X; SX2 += X*X; }
  double EX = SX/N, VX = SX2/N - EX*EX;
  return sqrt(VX);
}

static double window_CC (ix_t *q, ix_t *p) {
  double N = MAX(1,p-q), SX = 0, SY = 0, SXY = 0, SX2 = 0, SY2 = 0;
  double X0 = p->i, Y0 = p->x, X, Y;
  while (++q <= p) {
    X = q->i - X0; SX += X; SX2 += X*X; 
    Y = q->x - Y0; SY += Y; SY2 += Y*Y; SXY += X*Y; 
  }
  double EX = SX/N, VX = SX2/N - EX*EX;
  double EY = SY/N, VY = SY2/N - EY*EY;
  double EXY = SXY/N, COV = EXY - EX*EY;
  return COV ? (COV / sqrt(VX * VY)) : 0;
}

void f_window (ix_t *P, uint window, char aggregator) { // avg / min / max in window
  ix_t *p = P+len(P), *q;
  while (--p >= P) { // back-to-front
    uint T = p->i - window; // time of start of window
    for (q = p; q >= P && q->i >= T; --q); // q+1 is start of window
    ix_t *last = (p-1)>q ? p-1 : p;
    switch(aggregator) {
    case 'A': p->x = window_avg(q,last); break; // BP(..., p->x)
    case 'm': p->x = window_min(q,last); break;
    case 'M': p->x = window_max(q,last); break;
    case 'C': p->x = window_CC(q,p); break;
    }
  }
}

void f_anchor (ix_t *P, char anchor) { // open / close / day-min / day-max
  ix_t *p = P-1, *last = P+len(P)-1; float A = P->x, prev = P->x;
  while (++p <= last) {
    if (anchor != 'c' && beg_of_day(p,P)) A = p->x; // 
    prev = A; 
    switch(anchor) {
    case 'c': if (end_of_day(p,last)) A = p->x; break; // yesterday's close
    case 'M': if (p->x > A) A = p->x; break; // day max
    case 'm': if (p->x < A) A = p->x; break; // day min
    }
    p->x = prev; // use previous anchor, except on day-open
  }
}

void f_deltas (ix_t *P, char unit, char *intraday) { // unit = Basis,Log,Cents
  ix_t *p=P-1, *last = P+len(P)-1;
  float in = P->x;
  while (++p <= last) {
    float out = p->x;
    if (intraday && beg_of_day(p,P)) in = out;
    p->x = (unit == 'B' ? BP(in,out) :
	    unit == 'L' ? LP(in,out) : CP(in,out));
    in = out;
  }
}

void f_tt_EOD (ix_t *P) {
  ix_t *last = P+len(P)-1, *p = last+1, *eod = last;
  while (--p >= P) {
    if (end_of_day(p,last)) eod = p;
    p->x = ((double)eod->i - (double)p->i) / 3600; // exp((p->i - eod->i) / hl);
  }
}

void f_tt_BP (ix_t *P, float BP) { // time-to-BP
  float gain = bp2gain(BP);
  ix_t *p = P+len(P);
  while (--p >= P) {
    ix_t *q = p; float thresh = gain * p->x; // threshold
    if (BP>0) while (q >= P && q->x < thresh) --q;
    if (BP<0) while (q >= P && q->x > thresh) --q;
    p->x = (q>=P) ? log((double)p->i - (double)q->i) : 20;
  }
}

int ts_signals (char *SIG, char *PRC, char *prm) {
  uint wait = str2seconds (getprmp (prm,"wait=","0"));
  float ttBP = getprm (prm,"ttBP=",0);
  char *ttEOD = strstr (prm,"ttEOD");
  char *intraday = strstr (prm,"intraday");
  char *deltas = getprmp (prm,"deltas:","");
  char *type = getprmp (prm,"signals:","-");
  coll_t *P = open_coll (PRC, "r+"), *S = open_coll (SIG, "w+");
  uint id, n = num_rows(P);
  for (id = 1; id <=n; ++id) {
    ix_t *V = get_vec (P,id);
    if   (*deltas) f_deltas (V, *deltas, intraday);
    else if (ttEOD) f_tt_EOD (V);
    else if (ttBP) f_tt_BP (V, ttBP);
    else if (wait) f_window (V, wait, *type);
    else           f_anchor (V, *type);
    put_vec (S,id,V);
    free_vec(V);
    if (!(id%10)) show_progress (id, n, "rows");
  }
  free_coll (P); free_coll (S);
  return 0;
}

int ts_mpaste (char *TIC, char **_M, uint nM) {
  uint m; hash_t *H = open_hash (TIC, "r"); 
  ix_t **V = new_vec (nM, sizeof(ix_t*));
  coll_t **M = new_vec (nM, sizeof(coll_t*));
  printf ("TICK\tTimestamp");
  for (m = 0; m < nM; ++m) printf ("\t%s", _M[m]);
  printf ("\ttag\n");
  for (m = 0; m < nM; ++m) M[m] = open_coll (_M[m], "r+");
  uint r, nr = num_rows(M[0]);
  for (m = 0; m < nM; ++m) assert (num_rows(M[m]) == nr);
  for (r = 1; r <= nr; ++r) {
    char *tic = id2key(H,r), _[999];
    for (m = 0; m < nM; ++m) V[m] = get_vec_ro(M[m], r);
    uint c, nc = len(V[0]);
    for (m = 0; m < nM; ++m) assert (len(V[m]) == nc);
    for (c = 0; c < nc; ++c) {
      printf ("%s\t%s", tic, time2str(_,V[0][c].i));
      for (m = 0; m < nM; ++m) printf ("\t%.2f", V[m][c].x);
      //for (m = 0; m < nM; ++m) assert (V[m][c].i == V[0][c].i);
      char *tag = (beg_of_day(V[0]+c,V[0]) ? "\topen" :
    		   end_of_day(V[0]+c,V[0]+nc-1) ? "\tclose" : "\t");
      printf ("%s\n", tag);
    }
  }
  for (m = 0; m < nM; ++m) free_coll (M[m]);
  free_hash(H);
  return 0;
}

int ts_csv (char *_M, char *_H, char *prm) {
  char *fmt = strstr(prm,"round") ? "%c%.0f" : "%c%.4f";
  char tab = !strncmp(prm,"tsv",3) ? '\t' : ',';
  coll_t *M = open_coll(_M, "r+");
  hash_t *H = open_hash(_H, "r");
  uint id, n = num_rows(M);
  for (id = 1; id <= n; ++id) {
    char *tick = id2key(H,id), buf[99];
    ix_t *V = get_vec_ro(M,id), *last = V+len(V)-1, *v;
    if (id == 1) { 
      printf("Tick,Day");
      assert (V < last);
      for (v = V; v <= last; ++v) {
	printf ("%c%s", tab, time2strf(buf,"%T",v->i));
        if (end_of_day(v,last)) {
	  assert (v > V);
	  break;
	}
      }
    }
    for (v = V; v <= last; ++v) {
      if (beg_of_day(v,V)) printf("\n%s,%s", tick, time2strf (buf, "%F", v->i));
      printf(fmt, tab, v->x);
    }
  }
  printf("\n");
  free_coll(M); free_hash(H);
  return 0;
}

void ts_rank_eval(xy_t *R, char *prm) {
  double thr = getprm(prm,"thr=",40), sum = 0, x, y;
  uint i, n = len(R), pos = 0, neg = 0;
  for (i=1; i<n/2; ++i) {
    x = R[n-i].x;
    y = R[n-i].y;
    sum += y;
    if      (y >= +thr) ++pos;
    else if (y <= -thr) ++neg;
    if ((i > 100) && (sum / i < thr)) break;
  }
  printf(" %.4f %.0f %d %d+/%d/%d-\n", x, sum, i, pos, i-pos-neg, neg);
}

void ts_rank_tails(xy_t *R) {
  uint i, n = len(R)-1;
  double hsum = 0, tsum = 0; // cumulative sums over head and tail
  for (i=0; i<n/2; ++i) R[i].y = (hsum += R[i].y) / (i+1);   // head
  for (i=n; i>n/2; --i) R[i].y = (tsum += R[i].y) / (n-i+1); // tail
  // when many ranks share x-value, take y-value from the last rank
  for (i=n/2; i>0; --i) if (R[i-1].x == R[i].x) R[i-1].y = R[i].y; // head
  for (i=n/2; i<n; ++i) if (R[i+1].x == R[i].x) R[i+1].y = R[i].y; // tail
  // show values at top 100,200,... and bottom 100,200,...
  uint top[19] = {100,150,200,300,500,700,1000,1500,2000,3000,5000,7000,10000,15000,20000,30000,50000,70000,99999}, N = len(R)-1;
  for (i = 0; i < 19; ++i) printf(" %.0f", R[0+top[i]].y); // head ranks (small)
  for (i = 19; i > 0; --i) printf(" %.0f", R[N-top[i-1]].y); // tail ranks (big)
  printf ("\n");
}

int ts_rank (char *_SIG, char *_TRG, char *prm) {
  char *show = getprmp(prm,"rank:","viz");
  coll_t *TRG = open_coll (_TRG, "r+"), *SIG = open_coll (_SIG, "r+");
  uint i, j, nT = num_rows(TRG), nS = num_rows(SIG); assert (nS == nT);
  xy_t *R = new_vec(0,sizeof(xy_t));
  for (i = 1; i <= nT; ++i) {
    ix_t *T = get_vec_ro(TRG,i), *S = get_vec_ro(SIG,i);
    uint LT = len(T), LS = len(S); assert (LT == LS);
    for (j = 0; j < LT; ++j) {
      ix_t *s = S+j, *t = T+j; assert (s->i == t->i);
      xy_t new = {s->x, t->x};
      R = append_vec (R, &new);
    }
  }
  free_coll(TRG); free_coll(SIG);
  sort_vec(R, cmp_xy_x);
  printf ("%s %s", getprmp(_SIG,"/",_SIG), getprmp(_TRG,"/",_TRG));
  if (*show == 'v') ts_rank_tails(R);
  if (*show == 'e') ts_rank_eval(R,prm);
  return 0;
}

float *window_fn (uint n, char *type) {
  char *hann = strstr(type,"hann"), *welch = strstr(type,"welch");
  uint i = 0;
  float *W = new_vec(n,sizeof(float)), w;
  for (i=0; i<n; ++i) {
    double x = ((double)i)/n;
    if (hann)       { w = sin(PI*x); W[i] = w*w; }
    else if (welch) { w = 2*x - 1;   W[i] = 1-w*w; }
    else                             W[i] = 1;
  }
  return W;
}

int ts_windows (char *_RESULT, char *_SERIES, char *prm) {
  uint Len = getprm(prm,"len=",10);
  uint hop = getprm(prm,"hop=",1);
  coll_t *SERIES = open_coll(_SERIES,"r+");
  coll_t *RESULT = open_coll(_RESULT,"w+");
  uint i, nr = num_rows(SERIES), nw = 0, NW = 0, j;
  float *w = window_fn(Len,prm);
  ix_t *W = const_vec(Len,0);
  for (i=1; i<=nr; ++i) {
    ix_t *S = get_vec_ro(SERIES,i), *last = S+len(S)-Len, *s;
    if (i == 1) NW = nr * (last-S) / hop; // estimate
    for (s = S; s <= last; s+=hop) {
      for (j=0; j<Len; ++j) W[j].x = w[j] * s[j].x;
      put_vec(RESULT,++nw,W);
      if (!(nw%1000)) show_progress (nw, NW, "windows");
    }
  }
  free_coll(SERIES);
  free_coll(RESULT);
  free_vec(w);
  free_vec(W);
  return 0;
}

int ts_motifs (char *_MOTIFS, char *_SIM, char *prm) {
  uint far = getprm(prm,"far=",0);
  coll_t *SIM = open_coll(_SIM,"r+");
  jix_t *motifs = new_vec(0,sizeof(jix_t));
  float *SUPPORT = sum_rows (SIM, 1);
  uint nr = num_rows(SIM), nm = 0;
  while (1) {
    uint id = maxf(SUPPORT) - SUPPORT;
    if (!SUPPORT[id]) break;
    jix_t new = {++nm, id, SUPPORT[id]};
    motifs = append_vec (motifs, &new);
    ix_t *S = get_vec_ro(SIM,id), *s;
    for (s = S; s < S+len(S); ++s) SUPPORT[s->i] = 0; // remove similar
    uint a = (id <= far) ? 1 : (id-far), z = MIN(id+far,nr);
    for (id = a; id <= z; ++id) SUPPORT[id] = 0; // remove nearby
    show_progress(nm,nr,"motifs");
  }
  coll_t *MOTIFS = open_coll(_MOTIFS,"w+");
  append_jix (MOTIFS, motifs);
  MOTIFS->cdim = nr;
  free_coll(MOTIFS);
  free_coll(SIM);
  free_vec(motifs);
  free_vec(SUPPORT);
  return 0;
}

int ts_oplay (char *_PRICES, char *_TICK, char *prm) {
  float JUMP = getprm(prm,"jump=",100);
  float BACK = getprm(prm,"back=",0.5) * JUMP;
  char *LONG = strstr(prm,"long");
  coll_t *PRICES = open_coll(_PRICES,"r+");
  hash_t *TICK = open_hash(_TICK,"r");
  int i, nr = num_rows(PRICES);
  //printf ("%s\t%s\t\t%s\t\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
  //"BP","DAY","beg","stock","close","open","base","in","out","end");
  for (i = 1; i <= nr; ++i) {
    char *tick = id2key(TICK,i);
    ix_t *P = get_vec (PRICES,i), *last = P+len(P)-1, *p;
    float hi = 0, lo = 0, lo_open = 0, hi_open = 0;
    ix_t *min=P, *max=P, *open=P, *close=P, *in=0, *out=0;
    for (p = P; p <= last; ++p) {
      if (beg_of_day(p,P)) {
	min = max = open = p;
	in = out = 0;
	close = p>P ? p-1 : p;
	lo_open =  LONG && (BP(close->x,open->x) < -JUMP);
	hi_open = !LONG && (BP(close->x,open->x) > +JUMP);
      }
      if (p->x > max->x) max = p;
      if (p->x < min->x) min = p;
      if (lo_open && !in && BP(min->x,p->x) > +BACK) { in = p; hi = addBP(p->x,+JUMP); lo = addBP(p->x,-BACK); } // hi = close->x; lo = min->x; }
      if (hi_open && !in && BP(max->x,p->x) < -BACK) { in = p; hi = addBP(p->x,+BACK); lo = addBP(p->x,-JUMP); } // hi = max->x; lo = close->x; }
      if (in && !out && p->x >= hi) {p->x = hi; out = p;} // LMT order
      if (in && !out && p->x <= lo) {p->x = lo; out = p;} // LMT order
      //if (in && !out && (p->x >= hi || p->x <= lo)) out = p; // STP order
      if (in && end_of_day(p,last)) {
	if (!out) out = p;
	float bp = LONG ? BP(in->x,out->x) : -BP(in->x,out->x);
	ix_t *ref = LONG ? min : max;
	char x1[12], x2[9], x3[9];
	char *day = time2strf(x1,"%F",in->i);
	char *_in = time2strf(x2,"%T",in->i);
	char *_out= time2strf(x3,"%T",out->i);
	//char *I = time2str(x1,in->i), *O = time2str(x2,out->i);
	printf ("%.0f\t%s\t%s\t%s\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%s\n",
		bp, day, _in, tick, close->x, open->x, ref->x, in->x, out->x, _out);
      }
    }
    free_vec(P);
  }
  free_coll(PRICES); free_hash(TICK);
  return 0;
}

/*
int ts_oplay2 (char *_MKT, char *_TICK, char *prm) {
  float JUMP = getprm(prm,"jump=",100);
  float BACK = getprm(prm,"back=",0.5) * JUMP;
  char *LONG = strstr(prm,"long");
  hash_t *TICK = open_hash(_TICK,"r");
  coll_t *MKT = open_coll(_MKT,"r+");
  uint *t, *T = row_ids(MKT), *lastT = T+len(T)-1; // times
  uint s, ns = num_cols(MKT), sz = sizeof(float); // stocks
  float *open  = new_vec(ns+1,sz), *close = new_vec(ns+1,sz);
  float *min   = new_vec(ns+1,sz), *max   = new_vec(ns+1,sz);
  char *buy = new_vec(ns+1,1), *sell = new_vec(ns+1,1);
  float *in = new_vec(ns+1,sz), *out = new_vec(ns+1,sz);
  float *hi = new_vec(ns+1,sz), *lo = new_vec(ns+1,sz);
  for (t = T; t <= lastT; ++t) { // *t is a time when we have a quote
    uint t_open  = (t == T) || (t[0] - t[-1] > OVERNIGHT);
    uint t_close = (t == lastT) || (t[1] - t[0] > OVERNIGHT);
    ix_t *Q = get_vec_ro(MKT,*t); // quotes for all tickers at time *t
    float *P = vec2full (Q, ns, 0);
    for (s = 1; s <= ns; ++s) {
      if (t_open) {
	if (t == T) close[s] = P[s]; // special case
	min[s] = max[s] = open[s] = P[s];
	in[s] = out[s] = hi[s] = lo[s] = 0; 
	buy[s]  =  LONG && (BP(close[s],open[s]) < -JUMP); // low open: go long
	sell[s] = !LONG && (BP(close[s],open[s]) > +JUMP); // hi open: go short
      }
      if (P[s] < min[s]) min[s] = P[s];
      if (P[s] > max[s]) max[s] = P[s];
      if (!in[s] && buy[s]  && BP(min[s],P[s]) > +BACK) { in[s] = P[s]; hi[s] = close[s]; lo[s] = min[s]; }
      if (!in[s] && sell[s] && BP(max[s],P[s]) < -BACK) { in[s] = P[s]; hi[s] = max[s]; lo[s] = close[s]; }
      if (in[s] && !out[s] && P[s] <= lo[s]) out[s] = lo[s]; // out[s] ... STP order
      if (in[s] && !out[s] && P[s] >= hi[s]) out[s] = hi[s]; // LMT order
      if (in[s] && t_close) {
	if (!out[s]) out[s] = P[s];
	float bp = LONG ? BP(in[s],out[s]) : -BP(in[s],out[s]);
	float ref = LONG ? min[s] : max[s];
	char *tick = id2key(TICK,s);
	char _[15], *day = time2strf(_,"%F",*t);
	printf ("%.0f\t%s\t%s\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\n",
		bp, day, tick, close[s], open[s], ref, in[s], out[s]);
      }
      if (t_close) close[s] = P[s];
    }
    free_vec(P);
  }
  free_coll(MKT); free_hash(TICK);
  free_vec(open); free_vec(close); free_vec(min); free_vec(max);
  free_vec(in); free_vec(out);     free_vec(hi); free_vec(lo);
  free_vec(buy); free_vec(sell);
  return 0;
}
*/

/*
int ts_merge (char *_TRG, char *_SRC, char *_TIC, char *prm) {
  coll_t *TRG = open_coll(_TRG,"a+");
  coll_t *SRC = open_coll(_SRC,"r+");
  hash_t *TIC = open_hash(_TIC,"r");
  int i, nT = num_rows(TRG), nS = num_rows(SRC); 
  for (i = 1; i <= nS; ++i) {
    if (!has_vec(TRG,i)) ; // WAT DO?
    if (!has_vec(SRC,i)) ; // WAT DO?
    ix_t *X = get_vec_ro(TRG,i); 
    ix_t *Y = get_vec_ro(SRC,i); 
    ixy_t *S = join(X,Y,0), *s;
    for (s = S; s < S+len(S); ++s) {
      if (s->x && s->y && ABS(s->x - s->y) > 0.01) {
	char *tic = id2key(TIC,i), buf[99];
	char *time = time2str(buf,s->i);
	printf ("%s %s %.2f <> %.2f\n", tic, time, s->x, s->y);
      }
      else if (s->y && !s->x) s->x = s->y;
    }
    ix_t *T = ixy_to_ix_and_free(S);
    put_vec(TRG,i,T);
    free_vec(T);
  }
  free_coll(TRG); free_coll(SRC); free_hash(TIC);
}
*/

char *usage =
  "ts T = targets:gain=BP,loss=BP,trail=^|v,wait=7h,exits,binary PRICES\n"
  "ts C = codes:bits=10,day PRICES\n"
  "ts S = signals:close|open|min|Max|Avg|CC,wait=0 PRICES\n"
  "ts S = deltas:BP|LP|CP,intraday PRICES\n"
  "ts W = windows:len=10,hop=1 SERIES\n"
  "ts M = motifs SIMS\n"
  "ts signs PRICES TICK\n"
  "ts rank SIG TRG\n"
  "ts oplay PRICES TICK\n"
  //  "ts osim PRICES.T TICK\n"   // too slow, replaced by perday.awk
  "ts paste TICK M1 ... MN\n"
  "ts csv:prm M TICK\n"
  ;

#define arg(i) ((i < argc) ? argv[i] : NULL)
#define a(i) ((i < argc) ? argv[i] : "")

int main (int argc, char *argv[]) {
  vtime();
  if (!strncmp(a(1),"signs",5)) return ts_signs(arg(2),arg(3));
  if (!strncmp(a(1),"oplay",5)) return ts_oplay(arg(2),arg(3),a(1));
  //if (!strncmp(a(1),"osim",4)) return ts_oplay2(arg(2),arg(3),a(1));
  if (!strncmp(a(1),"rank",4)) return ts_rank(arg(2),arg(3),a(1));
  if (!strcmp(a(1),"paste")) return ts_mpaste(arg(2),argv+3,argc-3);
  if (!strncmp(a(1),"csv",3) || !strncmp(a(1),"tsv",3)) return ts_csv(arg(2),arg(3),a(1));
  if (argc < 4) { printf ("%s", usage); return 1; }
  else if (!strncmp(a(3), "targets", 7)) return ts_targets (arg(1), arg(4), arg(3));
  else if (!strncmp(a(3), "signals", 7)) return ts_signals (arg(1), arg(4), arg(3));
  else if (!strncmp(a(3), "deltas", 5)) return ts_signals (arg(1), arg(4), arg(3));
  else if (!strncmp(a(3), "codes", 5)) return ts_codes (arg(1), arg(4), arg(3));
  else if (!strncmp(a(3), "windows", 6)) return ts_windows (arg(1), arg(4), arg(3));
  else if (!strncmp(a(3), "motifs", 5)) return ts_motifs (arg(1), arg(4), arg(3));
  return 0;
}


/*
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
*/

/* -------------------- FLATTEN + MERGE -------------------- *\

POSTPONED: for now use: mtx print ... | sed | mtx load ...

uint *renumber_ids(uint *D) { // column id -> sequential number
  uint *d = D-1, *end = D+len(D), n=0;
  while (++d < end) if (*d) *d = ++n; // new sequential ids
  return D; // new_id = D[old_id]
}

uint *inverse_map(uint *A) { // in: mapping A:b->a
  uint b, nB = len(A)-1, nA = 0; // out: inverse B:a->b
  for (b=0; b <= nB; ++b) if (A[b] > nA) nA = A[b];
  uint *B = new_vec (nA+1,sizeof(uint));
  for (b=0; b <= nB; ++b) B[A[b]] = b; // if A[b]=a then B[a]=b
  return B; 
}

int ts_flatten (char *FLAT, char *SRC) {
  coll_t *F = open_coll(FLAT,"w+"), *S = open_coll(SRC,"r+");
  uint *col = renumber_cols (C), i, j, n = num_rows(S);
  for (i = 1; i<=n; ++i) {
    
  }
  free_vec (col); free_coll(F); free_coll(S);
  return 0;
}
\* ---------------------------------------- */

/* 
typedef struct {
  float gain; // limit on gain
  float loss; // limit on loss
  char trail; // trail up (^) or down (v)
  uint moc;   // close at end of day
} trg_t;
*/

/* -------------------- matching specific patterns -------------------- *\

SUPERSEDED: in favor of: price2signs + strstr(pattern)

uint x_pattern (ix_t *p, ix_t *last, char *s) {
  for (; *s && p < last; ++s, ++p) 
    if      (*s == '+' && (p->x >= (p+1)->x)) return 0;
    else if (*s == '-' && (p->x <= (p+1)->x)) return 0;
  return !*s;
}

void f_pattern (ix_t *P, char *pattern) {
  uint n = strlen(pattern);
  ix_t *p = P-1, *last = P+len(P)-1, *eod = P, *q = P;
  while (++p <= last) {
    if (end_of_day(p,last)) eod = p; 
    p->x = x_pattern(p-n,
  }
}
\* ---------------------------------------- */

/* -------------------- event run-length, EMA, event counts -------------------- *\

POSTPONED

  double A = strchr("aMm",aggregator) ? p->x : 0;
  ix_t *q = strchr("+-",aggregator) ? p-1 : p; // 
  case '^': while (--q >= P && q->i > T) A += (floor(q->x) < floor(q[1].x)); break; // $-crossing ^
  case 'v': while (--q >= P && q->i > T) A += (floor(q->x) > floor(q[1].x)); break; // $-crossing v
  case '+': while (--q >= P && q->i > T) A += (q->x < q[1].x && q[1].x < q[2].x); break; // double +
  case '-': while (--q >= P && q->i > T) A += (q->x > q[1].x && q[1].x > q[2].x); break; // double -

void f_runlen (ix_t *P, char event) { // run-of-events length
  ix_t *p = P+len(P);
  while (--p >= P) { // back-to-front
    ix_t *q = p;
    switch (event)
  }
}

void f_ema (ix_t *P, float a) { // EMA with half-life a
  float EMA = len(P) ? P->x : 0;
  ix_t *p = P-1, *last = P+len(P)-1;
  while (++p <= last) p->x = EMA = a * EMA + (1-a) * p->x;
}
\* ---------------------------------------- */

    //char buf[999];
    //printf ("open: %ld %d %s %.2f\n", (p-P), p->i, time2str(buf,p->i), p->x);

/* --------------- superceded by mpaste ---------------
int ts_paste (char *_A, char *_B, char *TIC) {
  coll_t *A = open_coll (_A, "r+"), *B = open_coll (_B, "r+");
  hash_t *H = open_hash (TIC, "r");
  uint id, n = num_rows(A);
  for (id = 1; id <= n; ++id) {
    char *tic = id2key(H,id), _[999];
    ix_t *a = get_vec_ro (A,id), *b = get_vec_ro (B,id);
    uint i, na = len(a), nb = len(b); assert (na == nb);
    for (i = 0; i < na; ++i) {
      assert (b[i].i == a[i].i);
      char *tag = (beg_of_day(a+i,a) ? "open" : end_of_day(a+i,a+na-1) ? "close" : "");
      printf ("%s %s %.2f %.2f %s\n", tic, time2str(_,a[i].i), a[i].x, b[i].x, tag);
    }
  }
  free_coll (A); free_coll (B); free_hash(H);
  return 0;
}
*/
