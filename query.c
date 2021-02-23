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

#include "hash.h"
#include "textutil.h"
#include "timeutil.h"
#include "spell.h"
#include "query.h"
#include "matrix.h"

qry_t *str2qry (char *str) {
  lowercase (str);
  csub (str, "\t\r\n", ' ');
  spaces2space (str);
  chop (str, " ");
  char inquote = 0, qop = '.';
  char **toks = split(str,' '); // space-separated lowercase tokens
  qry_t *Q = new_vec (len(toks), sizeof(qry_t)), *q;
  for (q = Q; q < Q+len(Q); ++q) {
    char *s = toks[q-Q], op = '.'; // token
    char *last = *s ? (s+strlen(s)-1) : s; // last character
    char *thr = s + strcspn(s,"<>="); // numeric threshold
    if (!inquote && index("-+.&",*s)) op = *s++; // operator
    if (!inquote && (*s == '\'')) { inquote = *s++; qop = op; } // begin quote
    if (!inquote && *thr) { q->type = *thr; q->thr = atof(thr+1); *thr = 0; }
    q->tok = s; // strdup(s)?
    q->op = inquote ? qop : op;
    q->type = q->type ? q->type : inquote ? inquote : '.';
    if ( inquote && (*last == '\'')) *last = inquote = 0; // end quote
  }
  free_vec(toks);
  return Q;  
}

void free_qry (qry_t *Q) {
  qry_t *q;
  for (q = Q; q < Q+len(Q); ++q) {
    if (q->docs) free_vec (q->docs);
    //if (q->children) free_qry (q->children);
    //if (q->tok) free (q->tok); // do not free: q->tok points inside buffer
  }
  free_vec (Q);
}

char *qry2str (qry_t *Q, hash_t *H) {
  char *buf=0; int sz=0;
  qry_t *q, *last = Q+len(Q)-1;
  for (q = Q; q <= last; ++q) {
    if (q->op == 'x') continue; // skip this term
    if (q > Q)                    zprintf (&buf,&sz, " ");
    if (index("-+",q->op))        zprintf (&buf,&sz, "%c", q->op);
    if (q->type== '\'')           zprintf (&buf,&sz, "'");
    if (q->type== '\'' || !q->id) zprintf (&buf,&sz, "%s", q->tok);
    else {
      if (q->id)                  zprintf (&buf,&sz, "%s", id2key(H,q->id));
      if (q->id2)                 zprintf (&buf,&sz, " %s", id2key(H,q->id2));
    }
    if (q->type== '\'')           zprintf (&buf,&sz, "'");
    if (index("<>=", q->type))    zprintf (&buf,&sz, "%c%.2f", q->type, q->thr);
  }
  return buf;
}

char *qry2original (qry_t *Q, hash_t *H) {
  char *buf=0; int sz=0;
  qry_t *q, *last = Q+len(Q)-1;
  for (q = Q; q <= last; ++q) {
    int changed = q->id && strcmp(q->tok, id2key(H,q->id));
    if (q->op == 'x') continue;
    if (q > Q)                      zprintf (&buf,&sz, " ");
    if (index("-+",q->op))          zprintf (&buf,&sz, "%c", q->op);
    if (q<last && (q+1)->op == 'x') zprintf (&buf,&sz, "'%s %s'", q->tok, (q+1)->tok);
    else if (changed)               zprintf (&buf,&sz, "'%s'", q->tok);
    else                            zprintf (&buf,&sz, "%s", q->tok);
  }
  return buf;
}

int qry_altered (qry_t *Q, hash_t *H) {
  qry_t *q, *last = Q+len(Q)-1;
  for (q = Q; q <= last; ++q) {
    char *term = q->id ? id2key(H,q->id) : NULL;
    if (term && strcmp(term,q->tok)) return 1;
  }
  return 0;  
}

char **toks4snippet (qry_t *Q, hash_t *H) { // words for snippet extraction  
  char **toks = new_vec (2*len(Q), sizeof(char*)), **t = toks;
  qry_t *q;
  for (q = Q; q < Q+len(Q); ++q) {
    if (q->op == 'x' || q->op == '-') continue; // exclude NOT and SKIP terms
    if (q->id)  *t++ = id2str(H,q->id);
    if (q->id2) *t++ = id2str(H,q->id2);
  }
  len(toks) = t - toks;
  return toks;
}

char *toks2str (char **toks) { // ' '.join(toks)
  char *buf=0; int sz=0, i=-1, n = len(toks);
  while (++i<n) if (toks[i]) zprintf (&buf,&sz, "%s ", toks[i]);
  if (sz) buf[--sz] = '\0'; // chop trailing space
  return buf;  
}

void spell_qry (qry_t *Q, hash_t *H, float *F, char *prm) {
  char V = prm && strstr(prm,"verbose") ? 1 : 0;
  qry_t *q, *last = Q+len(Q)-1;  
  for (q = Q; q <= last; ++q) q->id = has_key (H,q->tok); // pre-fetch  
  for (q = Q; q <= last; ++q) {    
    if (V) printf ("%c %c %s\n", q->op, q->type, q->tok);
    if (q->op == 'x') continue; // skip this term
    if (q->type == '\'') continue; // exact: do not correct    
    if (q < last) { // try fusing with next word
      uint wL = q->id, wR = (q+1)->id, wF = try_fuse (q->tok, (q+1)->tok, H);
      uint ok = (F[wF] > MIN(F[wL],F[wR]));
      if (V) printf ("0 %d\tfused: %s:%.0f + %s:%.0f -> %s:%.0f\n",
		     ok, q->tok, F[wL], (q+1)->tok, F[wR], id2key(H,wF), F[wF]);	
      if (ok) { q->id = wF; (q+1)->op = 'x'; /* skip q+1 */ continue; }
    }
    q->id = pubmed_spell (q->tok, H, F, prm, 0, &(q->id2));
  }
}

ix_t *exec_qry (qry_t *Q, hash_t *H, coll_t *INVL, char *prm) {
  double T = mstime();
  char V = prm && strstr(prm,"verbose") ? 1 : 0;  
  qry_t *q; ix_t *result = NULL;
  for (q = Q; q < Q+len(Q); ++q) {
    if (q->op == 'x') continue; // skip this term
    if (!q->id) q->id = has_key (H,q->tok);
    q->docs = get_vec (INVL, q->id);
    //if (q->children) q->docs = exec_qry (q->children, H, INVL);
    if (q->id2) { // term was a runon => intersect (L,R)
      ix_t *tmp = get_vec_ro (INVL, q->id2);
      filter_and_sum (q->docs, tmp);
    }
    if (!result) result = copy_vec (q->docs); // 1st term cannot be a negation
    else if (q->op == '.') filter_and_sum (result, q->docs);
    else if (q->op == '-') filter_not (result, q->docs);
    else if (q->op == '+') filter_sum (&result, q->docs);
    if (V) printf ("\t%.0fms %c%s -> [%d] -> [%d]\n",
		   msdiff(&T), q->op, id2key(H,q->id), len(q->docs), len(result));
  }
  return result ? result : new_vec(0,sizeof(ix_t));
}
 
#ifdef MAIN

int dump_parsed_qry (char *qry) {
  qry_t *Q = str2qry (qry), *q;
  //fprintf(stderr, "input: %d\n", len(Q));
  for (q = Q; q < Q + len(Q); ++q)
    printf ("%c\t%c\t%s\t%.2f\n", q->op, q->type, q->tok, q->thr);
  free_qry(Q);
  return 0;
}

int dump_spelled_qry (char *qry, char *_H, char *_F, char *prm) {
  qry_t *Q = str2qry (qry);
  hash_t *H = open_hash (_H,"r");
  float *F = mtx_full_row (_F, 1);  
  //expose_spell (Q, H, F, prm);
  spell_qry (Q, H, F, prm);
  char *str = qry2str (Q, H);
  printf("'%s'\n", str);
  free_qry(Q);
  free_hash(H);
  free_vec(F);
  free(str);
  return 0;
}

int do_exec_qry (char *qry, char *_W, char *_WxD, char *_DxX, char *prm) {
  qry_t *Q = str2qry (qry);
  hash_t *W = open_hash (_W,"r");
  coll_t *WxD = open_coll (_WxD,"r+");
  coll_t *DxX = open_coll (_DxX,"r");
  ix_t *D = exec_qry (Q, W, WxD, prm), *d, *end = D + MIN(5,len(D));
  sort_vec (D, cmp_ix_X);
  for (d=D; d < end; ++d) {
    char *xml = get_chunk(DxX,d->i);
    printf ("%.4f\n%s\n", d->x, xml);
  }
  free_vec(D);
  free_qry(Q);
  free_hash(W);
  free_coll(WxD);
  free_coll(DxX);
  return 0;
}

#define arg(i) ((i < argc) ? argv[i] : NULL)
#define a(i) ((i < argc) ? argv[i] : "")

char *usage = 
  "usage: query -parse 'rimantadin acid ic +\"aspirine\"'\n"
  "       query -spell 'rimantadin acid ic' WORD WORD_CF [verbose]\n"
  "       query -exec  'rimantadin acid ic' WORD WORDxDOC DOCxXML [verbose]\n"
  ;

int main (int argc, char *argv[]) {
  if (argc < 2) return fprintf (stderr, "%s", usage);
  if (!strcmp(a(1),"-parse")) return dump_parsed_qry(a(2));  
  if (!strcmp(a(1),"-spell")) return dump_spelled_qry(a(2), a(3), a(4), a(5));  
  if (!strcmp(a(1),"-exec")) return do_exec_qry(a(2), a(3), a(4), a(5), a(6));  
  return 0;
}

#endif

 /*
void spell_qry0 (qry_t *Q, hash_t *H, float *F, char *prm) {
  //float l1 = getprm (prm,"l1=",5);   // try 1-edit only if len(w0) > l1
  //float l2 = getprm (prm,"l2=",9);   // try 2-edit only if len(w0) > l2
  float f0 = getprm (prm,"f0=",100); // no spell-check if Freq[w0] > f0
  float x1 = getprm (prm,"x1=",10);  // prefer 1-edit if F[w1] > x1*F[w0]
  float x2 = getprm (prm,"x2=",100); // prefer 2-edit if F[w2] > x2*F[w0]
  float xI = getprm (prm,"xI=",10);  // prefer run-on if min F > xI*F[w0]
  qry_t *q; 
  for (q = Q; q < Q+len(Q); ++q) {
    printf ("%s [%c]\n", q->tok, q->type);
    if (q->type == 'x') continue; // skip this term
    
    uint w0 = has_key (H, q->tok), w1=0, w2=0, wL=0, wR=0, w01=0;
    if (q->type == '"') { q->id = w0; continue; } // exact: do not spell
    
    if (q+1 < Q+len(Q)) {
      w01 = try_fuse (q->tok, (q+1)->tok, H); // fuse w0 + next word
      printf ("\tfuse: %s:%.0f + %s -> %s:%.0f\n", q->tok, F[w0], (q+1)->tok, id2key(H,w01), F[w01]);
    }
    if (F[w01] > F[w0]) { q->id = w01; (q+1)->type = 'x'; continue; } // skip next term      
    
    printf ("\tas-is: %s:%.0f\n", q->tok, F[w0]);
    if (F[w0] > f0) { q->id = w0; continue; }
    
    best_edit (q->tok, 1, H, F, &w1);   // w1 = 1-edit(w0)
    printf ("\t1edit: %s:%.0f -> %s:%.0f\n", q->tok, F[w0], id2key(H,w1), F[w1]);
    if (F[w1] > x1*F[w0]) { q->id = w1; continue; }
    
    try_runon (q->tok, H, F, &wL, &wR); // wL,R = split(w0)
    printf ("\trunon: %s:%.0f -> %s:%.0f", q->tok, F[w0], id2key(H,wL), F[wL]);
    printf (" + %s:%.0f\n", id2key(H,wR), F[wR]);
    if (MIN(F[wL],F[wR]) > xI*F[w0]) { q->id = wL; q->id2 = wR; continue; }        
    
    best_edit (q->tok, 2, H, F, &w2);   // w2 = 2-edit(w0)
    printf ("\t2edit: %s:%.0f -> %s:%.0f\n", q->tok, F[w0], id2key(H,w2), F[w2]);
    if (F[w2] > x2*F[w0]) { q->id = w2; continue; }
    
    printf ("\t?????: %s:%.0f\n", q->tok, F[w0]);    
  }
}

void spell_qry1 (qry_t *Q, hash_t *H, float *F, char *prm) {
  (void) prm;
  qry_t *q, *last = Q+len(Q)-1;
  for (q = Q; q <= last; ++q) {
    qry_t *r = q < last ? (q+1) : NULL;
    printf ("%s [%c]\n", q->tok, q->type);
    if (q->type == 'x') continue; // skip this term
    
    uint w0 = has_key (H, q->tok), w1=0, w2=0, wL=0, wR=0, w0r=0, w11=0, w21=0;
    uint wr = r ? has_key (H, r->tok) : 0, l0 = strlen (q->tok);
    
    if (r) w0r = try_fuse (q->tok, r->tok, H); // fuse w0 + next word
    if (r) printf ("\tfused: %s:%.0f + %s:%.0f -> %s:%.0f\n", q->tok, F[w0], r->tok, F[wr], id2key(H,w0r), F[w0r]);
    if (F[w0r] > MIN(F[w0],F[wr])) { q->id = w0r; r->type = 'x'; printf ("\t^^^\n"); } // skip next term 
    
    printf ("\tas-is: %s:%.0f\n", q->tok, F[w0]);
    if (q->type == '"' || l0 < 5 || F[w0] > 1000) { q->id = w0; printf ("\t^^^\n"); } // do not spell
        
    best_edit (q->tok, 1, H, F, &w1); // w1 = 1-edit(w0)
    printf ("\t1edit: %s:%.0f -> %s:%.0f\n", q->tok, F[w0], id2key(H,w1), F[w1]);
    if (F[w1] > 10*F[w0]) { q->id = w1; printf ("\t^^^\n"); }
    
    if (w1) best_edit (id2key(H,w1), 1, H, F, &w11);
    if (w1) printf ("\t1-1ed: %s:%.0f -> %s:%.0f\n", q->tok, F[w0], id2key(H,w11), F[w11]);
    if (F[w11] > F[w1] && F[w11] > 10*F[w0]) { q->id = w11; printf ("\t^^^\n"); }
    
    if (l0 > 7) {
      
      best_edit (q->tok, 2, H, F, &w2);   // w2 = 2-edit(w0)
      printf ("\t2edit: %s:%.0f -> %s:%.0f\n", q->tok, F[w0], id2key(H,w2), F[w2]);
      if (F[w2] > 10*F[w1] && F[w2] > 10*F[w0]) { q->id = w2; printf ("\t^^^\n"); }
      
      if (w2) best_edit (id2key(H,w2), 1, H, F, &w21);
      if (w2) printf ("\t2-1ed: %s:%.0f -> %s:%.0f\n", q->tok, F[w0], id2key(H,w21), F[w21]);
      if (F[w21] > 10*F[w2] && F[w21] > 10*F[w0]) { q->id = w21; printf ("\t^^^\n"); }
    
      if (!w1) {
	try_runon (q->tok, H, F, &wL, &wR); // wL,R = split(w0)
	printf ("\trunon: %s:%.0f -> %s:%.0f", q->tok, F[w0], id2key(H,wL), F[wL]);
	printf (" + %s:%.0f\n", id2key(H,wR), F[wR]);
	if (MIN(F[wL],F[wR]) > 500 || !w2) { q->id = wL; q->id2 = wR; printf ("\t^^^\n"); }
      }
    }
  }
}

void expose_spell (qry_t *Q, hash_t *H, float *F, char *prm) {
  (void) prm;
  qry_t *q, *last = Q+len(Q)-1;
  for (q = Q; q <= last; ++q) {
    qry_t *r = q < last ? (q+1) : NULL;
    printf ("%s [%c]\n", q->tok, q->type);
    if (q->type == 'x') continue; // skip this term
    
    uint w0 = has_key (H, q->tok), w1=0, w2=0, wL=0, wR=0, w0r=0, w11=0, w21=0;
    uint wr = r ? has_key (H, r->tok) : 0;
    
    if (r) w0r = try_fuse (q->tok, r->tok, H); // fuse w0 + next word
    try_runon (q->tok, H, F, &wL, &wR); // wL,R = split(w0)
    best_edit (q->tok, 1, H, F, &w1);   // w1 = 1-edit(w0)    
    best_edit (q->tok, 2, H, F, &w2);   // w2 = 2-edit(w0)
    if (w1) best_edit (id2key(H,w1), 1, H, F, &w11);
    if (w2) best_edit (id2key(H,w2), 1, H, F, &w21);

    float f0 = w0 ? F[w0] : 1;
    float pf0 = (w0 && wr) ? MIN(F[w0],F[wr]) : 1;
    float pf = r ? F[w0r] / pf0 : 0;
    float p1 = 0.1 * F[w1] / f0;
    float p2 = 0.01 * F[w2] / f0;
    float p11 = 0.01 * F[w11] / f0;
    float p21 = 0.01 * F[w21] / f0;
    float pLR = 0.1 * MIN(F[wL],F[wR]) / f0;
    
    if (r) printf ("\t%5.0f fused: %s:%.0f + %s:%.0f -> %s:%.0f\n", pf, q->tok, F[w0], r->tok, F[wr], id2key(H,w0r), F[w0r]);
    printf ("\t%5.0f 1edit: %s:%.0f -> %s:%.0f\n", p1, q->tok, F[w0], id2key(H,w1), F[w1]);
    printf ("\t%5.0f 2edit: %s:%.0f -> %s:%.0f\n", p2, q->tok, F[w0], id2key(H,w2), F[w2]);
    printf ("\t%5.0f 1-1ed: %s:%.0f -> %s:%.0f\n", p11, q->tok, F[w0], id2key(H,w11), F[w11]);
    printf ("\t%5.0f 2-1ed: %s:%.0f -> %s:%.0f\n", p21, q->tok, F[w0], id2key(H,w21), F[w21]);
    printf ("\t%5.0f runon: %s:%.0f -> %s:%.0f", pLR, q->tok, F[w0], id2key(H,wL), F[wL]);
    printf (" + %s:%.0f\n", id2key(H,wR), F[wR]);
  }  
}
*/
