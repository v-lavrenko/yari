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

#include "math.h"
#include "matrix.h"
#include "textutil.h"

#define Inf 999999999

void dump_raw_ret (char *C, char *RH) {
  coll_t *c = open_coll (C, "r+");
  hash_t *h = open_hash (RH, "r!");
  char qryid[9999], docid[999], line[10000], *eol;
  while (fgets (line, 9999, stdin)) {
    if (*line == '#') continue;
    if ((eol = strchr(line,'\n'))) *eol = '\0';
    if (2 != sscanf (line, "%s %s", qryid, docid)) continue;
    uint i = key2id(h,docid);
    char *raw = get_chunk(c,i);
    printf ("%s\t%s\n",line,raw);
  }
  free_coll(c); free_hash(h);
}

void dump_rnd (char *C, char *prm) {
  srandom(time(0));
  coll_t *c = open_coll (C, "r+");
  uint l = getprm(prm,"len=",100); (void) l;
  uint num = getprm(prm,"num=",5)+1;
  uint N = nvecs(c);
  while (--num > 0) {
    uint i = 1 + random() % N;
    char *s = get_chunk(c,i);
    s += strcspn (s,">") + 1;
    char *end = strrchr(s,'<');
    fwrite (s, 1, end-s, stdout);
    fwrite ("\n", 1, 1, stdout);
    //fprintf(stderr,"doc %d\n",i);
  }
  free_coll(c);
}

void dump_raw (char *C, char *RH, char *id) {
  coll_t *c = open_coll (C, "r+");
  hash_t *h = *RH ? open_hash (RH, "r") : NULL;
  uint no = id ? getprm(id,"no=",0) : 0;
  uint i = no ? no : *id ? key2id(h,id) : 1;
  uint n = *id ? i : nvecs(c);
  //printf ("%s %s %d %d\n", C, RH, i, n);
  for (; i <= n; ++i) 
    if (has_vec (c,i))
      printf ("%s\n", (char*)get_chunk(c,i));
  free_coll(c); free_hash(h);
}

void load_raw (char *C, char *RH, char *prm) {
  char *addk = strstr(prm,"addkeys");
  uint done = 0;
  char *buf = malloc (1<<24);
  coll_t *c = open_coll (C, "w+");
  hash_t *rh = open_hash (RH, (addk ? "a!" : "r!"));
  while (read_doc (stdin, buf, 1<<24, "<DOC", "</DOC>")) {
    char *rowid = get_xml_docid (buf);
    uint id = key2id (rh, rowid), sz = strlen (buf) + 1;
    if (!id) fprintf(stderr,"UNKNOWN id: %s\n",rowid);
    free(rowid);
    if (!id) continue;
    put_chunk (c, id, buf, sz);
    if (!(++done%20000)) {
      if (done%1000000) fprintf (stderr, ".");
      else fprintf (stderr, "[%.0fs] %d strings\n", vtime(), done); 
    }
  }
  fprintf (stderr, "[%.0fs] %d strings\n", vtime(), nvecs(c));
  free_coll (c); free_hash (rh); free (buf);
}

void load_json (char *C, char *RH, char *prm) { // 
  char *skip = strstr(prm,"skip"), *join = strstr(prm,"join");
  char *addk = strstr(prm,"addkeys");
  uint done = 0, nodoc = 0, noid = 0, dups = 0, SZ = 1<<24;  
  char *json = malloc(SZ);
  coll_t *c = open_coll (C, "a+");
  hash_t *rh = open_hash (RH, (addk ? "a!" : "r!"));
  while (fgets (json, SZ, stdin)) { // assume one-per-line
    if (!(++done%10000)) show_progress (done, 0, "JSON records");
    uint sz = strlen (json);
    if (json[sz-1] == '\n') json[--sz] = '\0';
    char *docid = json_value (json, "docid"); 
    if (!docid) docid = json_value (json, "id");
    if (!docid && ++nodoc < 5) { fprintf (stderr, "ERR: no docid in: %s\n", json); continue; }
    uint id = key2id (rh, docid); 
    free(docid);
    if (!id) { ++noid; continue; }
    char *old = get_chunk (c,id);
    if (old && skip) continue;
    if (old && join) { // append old JSON to new JSON
      char *close = endchr (json,'}',sz); if (!close) fprintf (stderr, "ERR: %s\n", json); // have: {new}{old}
      char *open = strchr (old,'{');      if (!open)  fprintf (stderr, "ERR: %s\n", old);  // want: {new,old}
      strncpy (close,open,SZ-(close-json));
      *close = ',';
      sz = strlen (json);
      ++dups;
    }
    //if (has_vec(c,id)) ++dups; else 
    put_chunk (c, id, json, sz+1);
  }
  free_coll (c); free_hash (rh); free(json);
  fprintf (stderr, "[%.0fs] OK: %d, noid: %d, dups: %d\n", 
	   vtime(), done, noid, dups);
}

static inline char *merge_blobs (char *buf, char *a, char *b) {
  uint aSZ = strlen(a), bSZ = strlen(b), sz = aSZ + bSZ + 1;
  if (len(buf) < sz) buf = resize_vec (buf, sz);
  memcpy (buf, a, aSZ);
  memcpy (buf+aSZ, b, bSZ);
  char *close = endchr (buf,'}',aSZ); if (close) *close = ',';
  char *open  = strchr (buf+aSZ,'{'); if (open)  *open  = ' ';
  assert (open && close);
  buf [sz-1] = 0; // null-terminate
  //fprintf (stderr, "\na[%d]:%s\nb[%d]:%s\nc[%d]:%s\n", aSZ, a, bSZ, b, sz, buf);
  return buf;
}

void merge_colls (char *_C, char *_A, char *_B) { // C = A + B
  char *buf = new_vec (1<<24, sizeof(char));
  coll_t *C = open_coll (_C, "w+");
  coll_t *A = open_coll (_A, "r+");
  coll_t *B = open_coll (_B, "r+");
  uint nA = nvecs(A), nB = nvecs(B), n = MAX(nA,nB), i;
  fprintf (stderr, "merge: %s[%d] + %s[%d] -> %s\n", _A, nA, _B, nB, _C);
  for (i=1; i<=n; ++i) {
    char *a = get_chunk(A,i), *b = get_chunk(B,i);
    if (a && b) buf = merge_blobs (buf,a,b);
    char *c = (a && b) ? buf : a ? a : b;
    uint sz = c ? (strlen(c)+1) : 0;
    if (c) put_chunk (C, i, c, sz);
    if (!(i%10)) show_progress (i,n,"blobs merged");
  }
  fprintf (stderr, "done: %s[%d]\n", _C, nvecs(C));
  free_coll(A); free_coll(B); free_coll(C); free_vec(buf);
}

/*
void assert_inv_map (uint *A, uint *B) {
  uint a, b, nA = len(A), nB = len(B);
  fprintf (stderr, "assert: A -> B\n"); for (a=1; a<=nA; ++a) { b = A[a-1]; if (b) assert ((b <= nB) && (B[b-1] == a)); }
  fprintf (stderr, "assert: B -> A\n"); for (b=1; b<=nB; ++b) { a = B[b-1]; if (a) assert ((a <= nA) && (A[a-1] == b)); }
}
uint *inv = backmap (map); assert_inv_map (map, inv);
for (j=1; j<=nj; ++j) {
  if (!(j%10)) show_progress (j,nj,"blobs rekeyed");
  i = inv[j-1];
  if (!i) continue; // key not in A[H]
  char *b = get_chunk(B,i);
  if (b) put_chunk (A, j, b, strlen(b)+1);
 } 
 free_vec(inv); 
*/

void rekey_coll (char *_A, char *_H, char *_B, char *_G, char *prm) { // A[j] = B[i] where H[j] = G[i]
  char *access = strstr(prm,"addnew") ? "a!" : "r!";
  uint *map = hash2hash (_G, _H, access);
  coll_t *A = open_coll (_A, "w+");
  coll_t *B = open_coll (_B, "r+");
  uint i, j, nB = nvecs(B); assert (nB <= len(map));
  fprintf (stderr, "rekey: %s[%d] -> %s using map[%d]\n", _B, nB, _A, len(map));
  for (i=1; i<=nB; ++i) {
    if (!(i%10)) show_progress (i,nB,"blobs rekeyed");
    j = map[i-1];
    if (!j) continue; // key not in A[H]
    char *b = get_chunk(B,i);
    if (b) put_chunk (A, j, b, strlen(b)+1);
  }
  fprintf (stderr, "done: %s[%d]\n", _A, nvecs(A));
  free_coll(A); free_coll(B); free_vec(map); 
}

void do_merge (char *A, char *H, char *B, char *G, char *prm) { // A[j] += B[i] where key = H[j] = G[i]
  char *A1 = fmtn(999,"%s.1.%d",A,getpid());
  char *A2 = fmtn(999,"%s.2.%d",A,getpid());
  rekey_coll (A1, H, B, G, prm);
  if (coll_exists (A)) {
    merge_colls (A2, A1, A);
    mv_dir (A2, A);
    rm_dir (A1);
  } else mv_dir (A1, A);
  free (A1); free (A2);
}

void do_merge2 (char *_A, char *_H, char *_B, char *_G, char *prm) { // A[j] += B[i] where H[j] = G[i]
  char *access = strstr(prm,"addnew") ? "a!" : "r!";
  uint *map = (_G&&_H) ? hash2hash (_G, _H, access) : NULL;
  char *buf = new_vec (1<<24, sizeof(char));
  coll_t *A = open_coll (_A, "a+");
  coll_t *B = open_coll (_B, "r+");
  //hash_t *H = *_H ? open_hash (_H, mode) : NULL;
  //hash_t *G = *_G ? open_hash (_G, "r") : NULL;
  uint nB = nvecs(B), nA = nvecs(A), i;
  fprintf (stderr, "merging %s[%d] += %s[%d]", _A, nA, _B, nB);
  if (map) fprintf (stderr, ", mapping ids: %s -> %s[%s]\n", _G, _H, access);
  else     fprintf (stderr, ", assuming ids are compatible\n");
  if (map && len(map) < nB) { fprintf (stderr, "ERROR: %s [%d] > [%d] in map\n", _B, nB, len(map));  assert (0); }
  for (i=1; i<=nB; ++i) {
    if (!(i%10)) show_progress (i,nB,"blobs merged");
    uint j = map ? map[i-1] : i; // if no map: same ids in A,B
    //uint j = (H&&G) ? id2id (G, i, H) : i;
    if (!j) continue; // not addnew and key not in A[H]
    char *a = get_chunk(A,j), *b = get_chunk(B,i);
    if (a && b) b = buf = merge_blobs (buf,a,b);
    if (b) put_chunk (A, j, b, strlen(b)+1);
  }
  free_coll(A); free_coll(B); free_vec(buf); free_vec(map); // free_hash(H); free_hash(G); 
  fprintf (stderr, "\n");
}

ix_t *do_qry (char *QRY, char *DICT, char *prm) {
  hash_t *dict = open_hash (DICT, "r");
  //printf ("%s\n", QRY);
  erase_between(QRY,"%20","",' '); // %20 is HTML space
  ix_t *qry = parse_vec_txt (QRY, NULL, dict, prm); // parse query
  //print_vec_txt (qry, dict, "query:", 0);
  free_hash (dict);
  return qry;
}

ix_t *do_ret (ix_t *qry, char *INVL, char *prm) {
  (void) prm;
  coll_t *invl = open_coll (INVL, "r+");
  ix_t *ret = cols_x_vec (invl, qry); // matching docs
  sort_vec (ret, cmp_ix_X);
  free_coll (invl);
  return ret;
}

ix_t *do_exp (ix_t *qry, ix_t *ret, char *DOCS, char *prm) {
  uint nd = getprm(prm,"nd=",20), nw = getprm(prm,"nw=",20);
  float qw = getprm(prm,"qw=",10);
  coll_t *docs = open_coll (DOCS, "r+");
  trim_vec (ret, nd); // top 10 docs
  //vec_x_num (ret, '*', 3); // slightly curved softmax
  softmax (ret); 
  ix_t *exp = cols_x_vec (docs, ret); 
  trim_vec (exp, nw); // top 30 words
  ix_t *new = vec_add_vec (qw, qry, 1, exp);
  free_vec (exp); free_vec (qry); free_coll (docs); 
  return new;
}

void do_sort (ix_t *ret, char *ORDER, char *prm) {
  char *asc = strstr(prm,"asc");
  coll_t *O = open_coll (ORDER, "r+");
  ix_t *ord = get_vec_ro(O,1);
  float *order = vec2full (ord, 0, 0);
  vec_x_full (ret, '=', order);
  sort_vec (ret, (asc ? cmp_ix_x : cmp_ix_X));
  free_vec (order); free_coll (O); 
}

void do_pairs (ix_t *rnk, char *RANKS, char *PAIRS) {
  char buf[1000], *RANKs = fmt (buf, "%s.%d", RANKS, getpid()); 
  coll_t *ranks = open_coll (RANKs, "w+");
  coll_t *pairs = open_coll (PAIRS, "r+");
  ix_t *D = copy_vec (rnk), *d, *s;
  for (d = D; d < D+len(D); ++d) d->x = d-D+1; // value = rank
  sort_vec (D, cmp_ix_i); // re-sort by id
  for (d = D; d < D+len(D); ++d) {
    ix_t *P = get_vec_ro (pairs, d->i); // docs relevant to d (if any)
    ix_t *S = vec_x_vec (D, '&', P); // ranks of relevant docs
    for (s = S; s < S+len(S); ++s) { s->i = s->x; s->x = 1; } // id = rank
    sort_vec (S, cmp_ix_i); // re-sort by id (now is rank)
    if (len(S)) put_vec (ranks, d->x, S);
    free_vec (S);
  }
  free_vec (D);
  free_coll (pairs);
  free_coll (ranks);
  mv_dir (RANKs, RANKS);
}

void do_out (ix_t *rnk, char *TEXT, char *RNDR){ // unsafe: system()
  ix_t *r;
  //hash_t *dict = (DICT && *DICT) ? open_hash (DICT, "r")  : NULL;
  //coll_t *docs = (DOCS && *DOCS) ? open_coll (DOCS, "r+") : NULL;
  coll_t *text = open_coll (TEXT, "r+");
  system("mkdir src");
  for (r = rnk; r < rnk + len(rnk); ++r) {
    uint rank = r-rnk+1;
    char *txt = get_chunk (text, r->i), path[1000];
    
    //printf ("# rank %6d doc %6d score %.4f\n", rank, r->i, r->x);
    if (RNDR && *RNDR) {
      sprintf (path, "src/%d.txt", rank);
      FILE *out = safe_fopen (path, "w");
      fprintf (out, "%s", txt); 
      fclose (out);
      
      sprintf (path, "%s src/%d.txt > src/%d.log 2>&1", RNDR, rank, rank);
      system (path);
    }
    
    txt = strdup (txt);
    csub (txt, "\r\n", ' '); //for (t = txt; *t; ++t) if (*t == '\n') *t = ' '; // chop newlines
    //erase_between (txt, "<DOC ", ">", ' ');
    printf ("<DOC id=\"%d\">", rank);
    printf ("%s\n", txt);
    free(txt); // we made a copy
    //    if (docs && dict) { // dump vectors from collection
    //      ix_t *doc = get_vec (docs, r->i), *d;
    //      sort_vec (doc, cmp_ix_X);
    //      printf ("<DOC id=\"%d\">", rank);
    //      for (d = doc; d < doc + len(doc); ++d) printf (" %s", id2key(dict,d->i));
    //      //printf ("%6d %10s %.6f\n", rank, id2key(dict,d->i), d->x);
    //      //printf ("# END\n");
    //      printf ("</DOC>\n");
    //      free_vec (doc);
    //    } else { // dump a copy of XML
      //    }
  }
  free_coll (text); 
  //free_coll (docs); 
  //free_hash (dict);
}

ixy_t *ranked_list (coll_t *rets, coll_t *rels, uint i) {
  ix_t *rel = get_vec (rels, i);
  ix_t *ret = get_vec (rets, i);
  ixy_t *rnk = join (ret, rel, 0); // x = system, y = truth
  sort_vec (rnk, cmp_ixy_X); // sort by decreasing system scores
  free_vec (rel);
  free_vec (ret);
  return rnk;
}

void init_centroid (ixy_t *rnk, ix_t *seeds, coll_t *docs) {
  ix_t *centr = cols_x_vec (docs, seeds); // mean of their vecs
  ixy_t *d;
  for (d = rnk; d < rnk+len(rnk); ++d) { // for each doc:
    ix_t *doc = get_vec (docs, d->i); 
    d->x = cosine (doc, centr); // similarity of doc to centr
    free_vec (doc);
  }
  free_vec (centr);
}

void init_average (ixy_t *rnk, ix_t *seeds, coll_t *docs) {
  //int top = getprm (prm,"top=",10), n = MIN (top,len(rnk));
  ixy_t *d;
  for (d = rnk; d < rnk+len(rnk); ++d) { // for each doc:
    ix_t *doc = get_vec (docs, d->i), *s;
    d->x = 0;
    for (s = seeds; s < seeds + len(seeds); ++s) {
      ix_t *centr = get_vec (docs, s->i);
      d->x += cosine (doc, centr) / len(seeds); // mean similarity
      free_vec (centr);
    }
    free_vec (doc);
  }
}

ix_t *ixy_to_ix (ixy_t *Z) ;

void init_ranks (ixy_t *rnk, coll_t *docs, char *prm) {
  ix_t *ret = ixy_to_ix (rnk);
  trim_vec (ret, getprm (prm,"k=",10)); // take top docs
  vec_x_num (ret, '=', 1./len(ret)); // uniform weighting
  if (strstr(prm,"avg")) init_average (rnk, ret, docs);
  if (strstr(prm,"cntr")) init_centroid (rnk, ret, docs);
  free_vec (ret);
}

void ddrag (ixy_t *rnk, ixy_t *seed, coll_t *docs, float thresh) {
  ixy_t *d, *end = rnk + len(rnk);
  ix_t *svec = get_vec (docs, seed->i); 
  for (d = seed+1; d < end; ++d) { // for all docs following seed
    ix_t *dvec = get_vec (docs, d->i);
    float sim = cosine (dvec, svec);
    if (sim > thresh && sim > ABS(d->x)) d->x = seed->y ? sim : -sim;
    free_vec (dvec);
  }
  free_vec (svec);
}

void sdrag (ixy_t *rnk, ixy_t *seed, coll_t *sims, float thresh) {
  ixy_t *d, *end = rnk + len(rnk);
  ix_t *_sim = get_vec (sims, seed->i); // similarity of seed to all
  float *sim = vec2full (_sim, num_rows (sims), 0);
  //fprintf (stderr, "dragging %d (%c):", seed->i, (seed->y ? '+' : '-'));
  for (d = seed+1; d < end; ++d) { // for all docs following seed
    assert (d->i < len(sim));
    float x = sim [d->i]; // similarity of d to seed
    if (x < thresh || x < ABS(d->x)) continue; // not similar enough
    //fprintf (stderr, " %d", d->i);
    d->x = seed->y ? x : -x;
  }
  //fprintf (stderr, "\n");
  free_vec (sim); free_vec (_sim);
}

void post_drag_rerank (ixy_t *rnk) {
  ixy_t *d, *end = rnk + len(rnk);
  for (d = rnk; d < end; ++d) 
    if (d->x > 0) d->x = d-rnk; // positive => keep current rank
    else          d->x = d-rnk + 1E6; // negative => end of list
  sort_vec (rnk, cmp_ixy_x); // NOTE: ascending order
}

void do_test (char *RELS, char *RETS, char *SIMS, char *prm) {
  coll_t *rels = open_coll (RELS, "r+"); // relevance judgments
  coll_t *rets = open_coll (RETS, "r+"); // old ranked lists
  //coll_t *docs = open_coll (DOCS, "r+"); // document vectors
  coll_t *sims = open_coll (SIMS, "r+");
  float thr = getprm (prm,"thr=",0), skp = getprm (prm,"skp=",0);
  int i, n = num_rows (rets);
  for (i = 1; i <= n; ++i) {
    float rel = getprm (prm,"rel=",1), non = getprm (prm,"non=",0);
    ixy_t *rnk = ranked_list (rets, rels, i), *r;
    //init_ranks (rnk, docs, prm);
    for (r = rnk; r < rnk+len(rnk); ++r) { // for each doc in ranking
      if (r->x < 0) continue; // dragged away => can't touch this...
      if (random() < skp * RAND_MAX) continue; // skip some drags
      if (( r->y && --rel >= 0) ||
	  (!r->y && --non >= 0)) sdrag (rnk, r, sims, thr);
    }
    post_drag_rerank (rnk);
    eval_dump_evl (stdout, i, rnk);
    free_vec (rnk);
  }
  free_coll (sims);
  //free_coll (docs);
  free_coll (rets);
  free_coll (rels);
}

typedef struct { uint id; ix_t *ret; float ap; } list_t;

ix_t *rerank (ix_t *ret, coll_t *sims, float top) {
  trim_vec (ret, top);
  vec_x_num (ret, '/', sum(ret));
  return cols_x_vec (sims, ret);
}

void update_lists (list_t *lists, float thresh) {
  list_t *x, *z = lists + len(lists) - 1; // z points to new list
  vec_x_num (z->ret, '>', thresh); // keep similar-enough items
  for (x = lists; x < z ; ++x) { // redistribute items in the lists
    ix_t *oldx = x->ret, *oldz = z->ret;
    x->ret = vec_x_vec (x->ret, '>', z->ret); // closer to x
    z->ret = vec_x_vec (z->ret, '>', x->ret); // closer to z
    free_vec (oldx); free_vec (oldz);
  }
}

list_t *best_list (list_t *lists, ix_t *rnk, ix_t *rel) {
  list_t *l, *end = lists + len(lists), *best = lists;
  for (l = lists; l < end; ++l) {
    ix_t *ret = vec_x_vec (rnk, '&', l->ret); // original ordering
    l->ap = AveP (ret, rel);
    printf ("list: %ld seed:%d docs:%d ap:%.4f\n", l-lists, l->id, len(l->ret), l->ap);
    if (l->ap > best->ap) best = l;
    free_vec (ret);
  }
  return best;
}

uint top_nonrel (list_t *l, ix_t *rel, int *rank) {
  uint id = 0;
  ixy_t *evl = join(l->ret,rel,0), *e = evl-1, *end = evl+len(evl);
  sort_vec (evl, cmp_ixy_X);
  while (++e < end) if (e->y == 0 && e->i != l->id) {
      id = e->i;
      *rank = e-evl+1;
      break;
    }
  free_vec (evl);
  return id;
}

void free_lists (list_t *lists) {
  list_t *l = lists-1, *end = lists + len(lists);
  while (++l < end) if (l->ret) free_vec (l->ret);
  memset (lists, 0, len(lists) * sizeof(list_t));
  free_vec (lists);
}

void do_test2 (char *RELS, char *RETS, char *SIMS, char *prm) {
  coll_t *rels = open_coll (RELS, "r+"); // relevance judgments
  coll_t *rets = open_coll (RETS, "r+"); // old ranked lists
  coll_t *sims = open_coll (SIMS, "r+");
  double MAP = 0;
  int i, n = num_rows (rets), rank=0;
  for (i = 1; i <= n; ++i) {
    float thr = getprm (prm,"thr=",0), non = getprm (prm,"non=",1);
    float top = getprm (prm,"top=",1);
    printf ("------------------------------ %d ------------------------------\n", i);
    ix_t *tmp = get_vec (rets, i), *rel = get_vec (rels, i);
    ix_t *ret = rerank (tmp, sims, top);
    list_t *lists = new_vec (1, sizeof(list_t)), new = {0,0,0}, *best = NULL;
    lists[0].ret = copy_vec (ret);
    while (--non >= 0) {
      best = best_list (lists, ret, rel);
      new.id = top_nonrel (best, rel, &rank);
      printf ("dragging %d from list %ld rank %d\n", new.id, best-lists, rank);
      new.ret = get_vec (sims, new.id);
      lists = append_vec (lists, &new); // add list to collection
      update_lists (lists, thr);
    }
    MAP += best->ap;
    free_lists (lists);
    free_vec (ret);
    free_vec (tmp);
    free_vec (rel);
  }
  printf ("MAP: %.4f\n", MAP / n);
  free_coll (sims);
  free_coll (rets);
  free_coll (rels);
}

void mst_rerank (ix_t *rnk, coll_t *docs) {
  ix_t *d, *b;
  for (d = rnk; d < rnk + len(rnk); ++d) {
    ix_t *D = get_vec (docs, d->i);
    float best = -Inf, bx = -Inf;
    for (b = d-1; b >= rnk; --b) {
      ix_t *B = get_vec (docs, b->i);
      float sim = cosine (B,D);
      if (sim > best) { best = sim; bx = b->x; }
      free_vec (B);
    }
    d->x = MAX(d->x,MIN(best,bx));
    free_vec (D);
  }
}

void do_mst (char *OUT, char *RETS, char *DOCS, char *prm) {
  coll_t *out  = open_coll (OUT,  "w+");
  coll_t *rets = open_coll (RETS, "r+");
  coll_t *docs = open_coll (DOCS, "r+");
  uint top = getprm(prm,"top=",100);
  uint q = 0, nq = num_rows (rets);
  while (++q <= nq) {
    ix_t *rnk = get_vec (rets, q);
    sort_vec (rnk, cmp_ix_X);
    if (len(rnk) > top) len(rnk) = top;
    mst_rerank (rnk, docs);
    sort_vec (rnk, cmp_ix_i);
    put_vec (out, q, rnk);
    free_vec(rnk); 
    show_progress (q, nq, "ranked lists");
  }
  free_coll (rets); free_coll (docs); free_coll (out);
}

////////////////////////////// normalize scores for better thresholding

void thr_irank (ix_t *V) { // score = inverse rank
  ix_t *v, *end = V+len(V);
  for (v = V; v < end; ++v) v->x = 1. / (v-V+1);
}

void thr_at_rank (ix_t *V, uint rank, uint num) { // score at rank
  float sum = 0;
  ix_t *v, *beg = MAX(V,V+rank-1), *end = MIN(V+len(V),beg+num);
  for (v = beg; v < end; ++v) sum += v->x;
  float thresh = sum ? (sum / (end-beg)) : 1;
  vec_x_num (V, '/', thresh);
}

void thr_prior (ix_t *vec, float prior) {
  float thresh = 0, max = 0, sum = 0;
  ix_t *v, *end = vec+len(vec);
  for (v = vec; v < end; ++v) {
    float x = v->x, rank = v-vec+1, obj = 0;
    if        (prior > 0) obj = x * rank / (rank+prior);
    //else if (prior == -1) obj = x * log(rank);
    else if (prior == -1) obj = x * rank / (sum+=x);
    if (obj > max) { max = obj; thresh = x; }
  }
  vec_x_num (vec, '/', thresh);
}

void thr_ttest (ix_t *vec) {
  uint N = MIN(100,len(vec)-1);
  ix_t *v, *beg=vec+1, *end = vec+N; // best = {0,0};
  double Lsum1 = 0, Rsum1 = 0, Lsum2 = 0, Rsum2 = 0;
  for (v = beg; v < end; ++v) {
    double x = v->x - (v+1)->x; Rsum1 += x;  Rsum2 += x*x;  }
  for (v = beg; v < end; ++v) {
    double x = v->x - (v+1)->x, nL = v-beg+1, nR = end-v-1;
    double Lmean = (Lsum1 += x) / nL, Rmean = (Rsum1 -= x) / nR;
    double Lm2 = (Lsum2 += x*x) / nL, Rm2 = (Rsum2 -= x*x) / nR;
    double Lvar = Lm2 - Lmean*Lmean, Rvar = Rm2 - Rmean*Rmean;
    if (0) printf ("%3.0f %.4f left: %.4f : %.4f +/- %.4f ... right: %.4f : %.4f +/- %.4f\n",
	    nL, x, 
	    Lsum1, Lmean, Lvar, 
	    Rsum1, Rmean, Rvar);
    double Z = (Lmean - Rmean);// / sqrt (Lvar/nL + Rvar/nR);
    //if (i==1) continue;
    //if (Z > best_z) { best_i = i; best_z = Z; }
    if (nL>1 && nR>1) printf ("%3.0f %.4f z=%.4f %.4f / %.4f <=> %.4f / %.4f\n", 
			      nL, x, Z, Lmean, Lvar, Rmean, Rvar);
  }
  //for (i=0; i<best_i; ++i) _vec[i].x += 1;
  //free_vec (vec);
}

void thr_break (ix_t *vec, double thr) {
  uint N = MIN(100,len(vec)-1);
  ix_t *v, *end = vec+N;
  double sx = 0, s2 = 0;
  for (v = end; v > vec; --v) {
    double x = (v-1)->x - v->x, n = end-v+1;
    double Ex = (sx += x) / n, Ex2 = (s2 += x*x) / n;
    double Vx = Ex2 - Ex*Ex, z = (x - Ex) / sqrt (Vx?Vx:1);
    if (n > N/2 && z > thr) break;
  }
  while (--v > vec) v->x = 0.99 * vec[1].x + 0.01 * v->x;
}

void thr_mad (ix_t *vec, double thr) {
  uint N = MIN(1000,len(vec)-1);
  ix_t *v, *end = vec+N;
  double med = vec[N/2].x, s = 0, n = 0;
  for (v = end; v > vec; --v) {
    double d = v->x - med, mad = (s += ABS(d)) / ++n;
    if (n > N/2 && d > thr*mad) break;
  }
  for (; v > vec; --v) v->x = 0.99 * vec[1].x + 0.01 * v->x;
}

void do_thr (char *OUT, char *RETS, char *prm) {
  char *inv = strstr (prm,"irank"), *tst = strstr(prm,"ttest");
  float brk = getprm (prm,"brk=",0), mad = getprm (prm,"mad=",0);
  uint rnk = getprm (prm,"rnk=",0), num = getprm (prm,"num=",1);
  //uint min = getprm (prm,"min=",0);
  float pri = getprm (prm,"prior=",0);
  coll_t *outs = open_coll (OUT,  "w+");
  coll_t *rets = open_coll (RETS, "r+");
  uint r = 0, nr = num_rows (rets);
  while (++r <= nr) {
    ix_t *ret = get_vec (rets, r);
    if (!len(ret)) { free_vec (ret); continue; }
    sort_vec (ret, cmp_ix_X);
    if (pri) thr_prior (ret, pri);
    if (rnk) thr_at_rank (ret, rnk, num);
    if (inv) thr_irank (ret);
    if (tst) thr_ttest (ret);
    if (brk) thr_break (ret,brk);
    if (mad) thr_mad (ret,mad);
    sort_vec  (ret, cmp_ix_i);
    put_vec (outs, r, ret);
    free_vec (ret);
    show_progress (r, nr, "ranked lists");
  }
  printf ("%d ranked lists\n", nr); 
  free_coll (rets); free_coll (outs);
}

//////////////////////////////////////////////////////////////////////
// -rnk : try out different drag models : supercedes -test and -test2
//////////////////////////////////////////////////////////////////////

void dump_svm_vec (float y, ix_t *vec, FILE *out) { 
  ix_t *v = vec-1, *vEnd = vec+len(vec);
  fprintf (out, "%.0f", y);
  while (++v < vEnd) fprintf (out, " %d:%.4f", v->i, v->x);
  fprintf (out, "\n");
}

void save_svm_vec (float y, ix_t *vec, char *path, char *mode) { 
  FILE *out = safe_fopen (path, mode); 
  dump_svm_vec (y, vec, out);
  fclose(out);
}

void save_svm_vecs (ix_t *set, coll_t *vecs, char *path, char *mode) { 
  FILE *out = safe_fopen (path, mode); ix_t *s;
  for (s = set; s < set + len(set); ++s) {
    ix_t *vec = get_vec (vecs, s->i);
    dump_svm_vec (s->x, vec, out);
    free_vec (vec);
  }
  fclose(out);
}

void load_svm_preds (ix_t *set, char *path) { 
  char line[999]; 
  ix_t *s = set, *sEnd = set + len(set);
  FILE *in = safe_fopen (path, "r"); 
  while (fgets (line, 999, in) ) 
    if (*line == 'l' || *line == '#') continue;
    else if (s < sEnd) s++->x = atof (line);
    else fprintf (stderr, "ERROR: extra line in %s: %s", path, line);
  while (s < sEnd) fprintf (stderr, "ERROR: no prediction for %d in %s\n", s++->i, path);
  fclose(in);
}

typedef struct { uint set; uint doc; float sim; float rel; } rnk_t;

coll_t *RELS = NULL, *RETS = NULL, *DOCS = NULL, *INVL = NULL, *SIMS = NULL, *SIMT = NULL;

ix_t *centroid (coll_t *vecs) {  // thread-unsafe: static
  static ix_t *avg = NULL;
  if (avg) return avg;
  uint N = num_rows (vecs);
  ix_t *set = const_vec (N, 1./N);
  avg = cols_x_vec (vecs, set);
  free_vec (set);
  return avg;
}

rnk_t *ixy_to_rnk (ixy_t *ixy, uint top) {
  top = MIN (top, len(ixy));
  ixy_t *i, *end = ixy+top;
  rnk_t *r, *rnk = new_vec (top, sizeof(rnk_t));
  for (i = ixy, r = rnk; i < end; ++i, ++r) {
    r->set = 0; r->doc = i->i; r->sim = -Inf; r->rel = i->y; }
  free_vec (ixy);
  return rnk;
}

uint rnk_count (char what, rnk_t *rnk, uint set) {
  uint count = 0; rnk_t *r, *end = rnk+len(rnk);
  for (r = rnk; r < end; ++r) 
    if      (what == 'r' && r->rel == set)                  ++count; // relevant to set
    else if (what == 's' && r->set == set)                  ++count; // currently in set
    else if (what == 'd' && r->set == set && r->sim == Inf) ++count; // dragged to set
    else if (what == 'm' && r->rel > count) count = r->rel; // max set id
  return count; 
}

ix_t *rnk_sets (rnk_t *rnk) {
  uint i, n = len(rnk);
  ix_t *sets = const_vec (n,1);
  for (i=0;i<n;++i) sets[i].i = rnk[i].rel;
  sort_vec (sets, cmp_ix_i);
  uniq_vec (sets);
  return sets;
}

uint rnk_max_set (rnk_t *rnk) { // max. set id in the ranking
  uint m = 0; rnk_t *r, *end = rnk+len(rnk);
  for (r = rnk; r < end; ++r) if (r->rel > m ) m = r->rel;
  return m;
}

uint rnk_num_sets (rnk_t *rnk) { // count sets with rel docs
  uint set, m = rnk_max_set (rnk), num = 0; // not counting set 0
  for (set = 1; set <= m; ++set) if (rnk_count ('r',rnk,set)) ++num;
  return num;
}

float rnk_eval_binary (rnk_t *rnk, char *type) {
  float relret = 0, rel = 0, ret = 0, ap = 0; 
  rnk_t *r;
  for (r = rnk; r < rnk+len(rnk); ++r) {
    if (r->rel == 1) ++rel;
    if (r->set == 1) ++ret;
    if (r->rel == 1 && r->set == 1) ap += (++relret / ret); 
  }
  if (*type == 'R') return relret / rel;
  if (*type == 'P') return relret / ret;
  if (*type == 'F') return 2 * relret / (rel + ret);
  if (*type == 'A') return ap / rel;
  return 0;
}

float rnk_eval_multi (rnk_t *rnk, char *type) {
  uint i, nsets = 0, t = *type, m = 1+rnk_max_set(rnk);
  float *rr  = new_vec (m,sizeof(float)), sumF1 = 0;
  float *rel = new_vec (m,sizeof(float)), sumR  = 0;
  float *ret = new_vec (m,sizeof(float)), sumP  = 0;
  float *ap  = new_vec (m,sizeof(float)), sumAP = 0;
  rnk_t *r; 
  for (r = rnk; r < rnk+len(rnk); ++r) {
    uint set = r->set, tru = r->rel, ok = (set == tru) ? 1 : 0;
    assert (r->rel < m && r->set < m);
    rel [tru] += 1; // number of relevant docs for set
    ret [set] += 1; // number of retrieved docs for set
    rr  [set] += ok; // relevant-retrieved docs for set
    ap  [set] += ok * rr[set] / ret[set]; // sum precisions at rel ranks
  }
  for (i = 1; i < m; ++i) if (rel[i]) { // start from 1 => skip set 0
      nsets += 1;
      sumR  += rr[i] / rel[i];
      sumP  += rr[i] / MAX(1,ret[i]);
      sumF1 += 2 * rr[i] / (rel[i] + ret[i]);
      sumAP += ap[i] / rel[i];
    }
  return ((t=='R')?sumR : (t=='P')?sumP : (t=='A')?sumAP : sumF1) / nsets;
}

ix_t *rnk_select (rnk_t *rnk, char *mode, uint set) {
  ix_t *result = new_vec (0, sizeof(ix_t));
  rnk_t *r;
  for (r = rnk; r < rnk + len(rnk); ++r) {
    ix_t new = {r->doc, 0};
    if      (*mode == 'd' && r->sim == Inf && r->set == set) new.x = r->set;
    else if (*mode == 'D' && r->sim == Inf)                  new.x = r->set;
    else if (*mode == 'r' && r->rel == set)                  new.x = r->sim;
    else if (*mode == 's' && r->set == set)                  new.x = r->sim;
    else if (*mode == 'a')                                   new.x = r->sim;
    else if (*mode == 'A')                                   new.x = r->rel;
    else if (*mode == 'E')                                   new.x = r->set;
    else if (*mode == 't' && (!set || r-rnk < set))          new.x = r->sim;
    else continue; // doesn't match don't append anything
    result = append_vec (result, &new);
  }
  return result;
}

void rnk_show_set (rnk_t *rnk, uint set) {
  rnk_t *r, *end = rnk+len(rnk); char c;
  float TP = 0, FP = 0, TN = 0, FN = 0;
  printf ("%c:", (set ? ('`'+set) : '-'));
  for (r = rnk; r < end; ++r) {
    if (r->set == set) { if (r->rel == set) ++TP; else ++FP; }
    else               { if (r->rel == set) ++FN; else ++TN; }
    if      (r->set != set) c = ' ';
    else if (r->sim == Inf) c = '#';
    else if (r->rel == 0)   c = '-';
    else                    c = '`' + r->rel;
    //else if (r->rel == set) c = set ? '+' : '-';
    //else                    c = set ? '-' : '+';
    if (r-rnk < 140) printf ("%c", c);
  }
  float R = TP ? (100*TP/(TP+FN)) : 0, P = TP ? (100*TP/(TP+FP)) : 0;
  float F1 = (P+R) ? (2*P*R/(P+R)) : 0;
  if (!set) printf (" %3.3s %3.3s %3.3s %3.3s\n", "TP", "FP", "FN", "F1");
  //else printf (" %.2f\n",  F1);
  else      printf (" %3.0f %3.0f %3.0f %3.0f\n",  TP, FP, FN, F1);
}

void rnk_show (rnk_t *rnk) {
  uint set, m = rnk_max_set(rnk);
  for (set = 0; set <= m; ++set) 
    if (!set || rnk_count ('r',rnk,set)) rnk_show_set (rnk,set);
}

float ret_thresh (ix_t *_ret, uint top) {
  //float max = -Inf, thresh = -Inf, sum = 0, obj;
  ix_t *ret = copy_vec (_ret);
  sort_vec (ret, cmp_ix_X);
  if (top+1 > len(ret)) top = len(ret)-1;
  float thresh = ret [top] . x;
  //for (r = ret; r < ret+len(ret); ++r) {
  //obj = log(r-ret+1) * r->x;
    //sum += r->x;
    //obj = (r-ret+1) * r->x / sum;
    //if (obj >= max) { max = obj; thresh = r->x; }
    //}
  free_vec (ret);
  return thresh;
}

ix_t *rnk_get_ret (coll_t *sims, uint doc, char *prm) {
  float knn = getprm (prm, "knn=", 1);// P = getprm (prm, "P=", 1);
  ix_t *nns = get_vec (sims, doc); // similarity of dragged doc to everything
  if (knn == 1) return nns; 
  trim_vec (nns, knn); // take top nearest neighbours with weights
  //vec_x_num (nns, '^', P); // smooth / sharpen the weights
  vec_x_num (nns, '/', sum(nns)); // make weights sum to 1
  ix_t *ret = cols_x_vec (sims, nns); // average similarity of nns to everything
  free_vec (nns);
  return ret;
}

void rnk_drag_1nn (rnk_t *rnk, rnk_t *r, uint set, char *prm) { 
  float thresh = getprm (prm, "sim=", -Inf); // similarity threshold
  uint top = getprm (prm, "top=", 10); // max neighbours to drag
  ix_t *ret = rnk_get_ret (SIMS, r->doc, prm); /// get_vec (sims, r->doc);   
  if (thresh == -Inf) thresh = ret_thresh (ret, top);
  printf ("t=%.2f\n", thresh);
  trim_vec (ret, top);
  for (r = rnk; r < rnk + len(rnk); ++r) {
    float sim = vec_get (ret, r->doc);
    if (sim > thresh && sim > r->sim) { r->sim = sim; r->set = set; }
  }
  free_vec (ret);
}

ix_t *nearest_docs (ix_t *set, int knn, rnk_t *rnk, uint rank) {
  if (!knn) return new_vec (0,sizeof(ix_t));
  ix_t *sims = cols_x_vec (SIMS, set); // average similarity
  ix_t *mask = rnk_select (rnk, "top", rank); // docs above rank
  sort_vec (mask, cmp_ix_i); // put mask in right order
  ix_t *ret = vec_x_vec (sims, '&', mask); // remove sims not in mask
  trim_vec (ret, knn); // near if knn > 0, far if knn < 0
  free_vec (sims); free_vec (mask);
  return ret;
}

ix_t *nearest_knn (ix_t *set, uint K, rnk_t *rnk) {
  ix_t *out = rnk_select (rnk, "a", 0), *d; // all docs we need
  for (d = out; d < out+len(out); ++d) { // for each doc 
    ix_t *tmp = get_vec (SIMT,d->i); // sim of all docs to this one
    ix_t *sim = vec_x_vec (tmp, '&', set); // remove docs not in set
    trim_vec (sim, K); // pick k nearest neighbours (from set)
    d->x = sum(sim) / MAX(1,len(sim)); // average of similarities
    free_vec (tmp); free_vec (sim);
  }
  sort_vec (out, cmp_ix_i); // put in right order for vec_get()
  return out;
}

void weigh_vec_irank (ix_t *vec) ;
ix_t *examples_for_set (uint set, rnk_t *rnk, char *prm) {
  int Near = getprm (prm, "near=", 0), Far = getprm (prm, "far=", 0);
  int rank = getprm (prm, "rank=", 0);
  float farw = getprm (prm, "farw=", 1);
  ix_t *pos = rnk_select (rnk,"dragged",set); vec_x_num (pos,'=',1); 
  if (Near && len(pos)) { // add docs nearest to positive examples
    float irank = getprm (prm, "irank=",0), p = getprm (prm,"p=",0); (void) irank;
    ix_t *near = nearest_docs (pos, Near, rnk, 0);
    //if (irank) weigh_vec_irank (near);
    if (p) vec_x_num (near, '^', p);
    ix_t *both = vec_x_vec (pos, '+', near);
    free_vec (pos); free_vec (near);
    pos = both;
    vec_x_num (pos, '/', sum(pos)); // make weight sum to 1
  }
  if (Far && !len(pos)) { // use docs farthest from negative examples
    ix_t *all = rnk_select (rnk,"Dragged",0);        vec_x_num (all,'=',1);
    ix_t *far = nearest_docs (all, -Far, rnk, rank); vec_x_num (far,'=',1);
    free_vec (pos); free_vec (all);
    pos = far; 
    vec_x_num (pos, '/', sum(pos)/farw); // make weights sum to farw
  }
  return pos;
}

void rnk_drag_svm (rnk_t *rnk, char *prm) { // thread-unsafe: centroid + system()
  uint set, m = rnk_max_set (rnk);
  save_svm_vec (0, centroid(DOCS), "./train.vecs", "w");
  for (set = 0; set <= m; ++set) {
    if (set && !rnk_count ('r',rnk,set)) continue;
    ix_t *pos = examples_for_set (set, rnk, prm); 
    vec_x_num (pos,'=',set); 
    save_svm_vecs (pos, DOCS, "./train.vecs", "a"); 
    free_vec (pos);
  }
  ix_t *test = rnk_select (rnk, "All docs", 0), *a;
  save_svm_vecs (test, DOCS, "./test.vecs", "w");  
  system ("./csvm.csh"); // test.vecs -> SVM -> test.predictions
  load_svm_preds (test, "./test.pred");
  rnk_t *r, *rEnd = rnk+len(rnk);
  for (r=rnk, a=test; r < rEnd; ++r, ++a) if (r->sim < Inf) r->set = a->x;
  free_vec (test); 
}

void rnk_drag_ann (rnk_t *rnk, char *prm) {
  float thresh = getprm (prm, "thresh=", -Inf), knn = getprm (prm,"knn=",0);
  uint set, m = rnk_max_set (rnk);
  for (set = 0; set <= m; ++set) {
    if (set && !rnk_count ('r',rnk,set)) continue;
    ix_t *pos = examples_for_set (set, rnk, prm); 
    ix_t *ret = knn ? nearest_knn(pos,knn,rnk) : nearest_docs(pos,len(rnk),rnk,0);
    rnk_t *r, *rEnd = rnk+len(rnk);
    for (r = rnk; r < rEnd; ++r) {
      float sim = vec_get (ret, r->doc);
      if (sim > thresh && sim > r->sim) { r->sim = sim; r->set = set; }
    }
    free_vec (pos); free_vec (ret);
  }
}

ix_t *qry_centr (ix_t *docs) {
  ix_t *qry = cols_x_vec (DOCS, docs); 
  vec_x_num (qry, '/', sum(docs));
  return qry;
}

ix_t *qry_diff (ix_t *pos) { // thread-unsafe: centroid
  ix_t *c = centroid (DOCS);
  ix_t *p = qry_centr (pos); // average of the positives
  ix_t *h = vec_x_vec (p, '-', c);
  free_vec(p); // don't free c, it's static
  return h;
}

ix_t *qry_logr (ix_t *docs, char *prm) { // thread-unsage: centroid
  float p = getprm (prm,"smooth=",0.9);
  ix_t *bg = centroid (DOCS);          vec_x_num (bg, '/', sum(bg));
  ix_t *lm = qry_centr (docs);
  ix_t *sm = vec_add_vec(p,lm,1-p,bg);
  ix_t *lr = vec_x_vec (sm,'/',bg);    vec_x_num (lr, 'l', 0);
  free_vec (lm); free_vec (sm); // don't free bg, it's static!
  return lr;
}

ix_t *qry_clarity (ix_t *docs, char *prm) { // thread-unsafe: centroid
  float p = getprm (prm,"smooth=",0.9);
  ix_t *bg = centroid (DOCS);          vec_x_num (bg, '/', sum(bg));
  ix_t *lm = qry_centr (docs);
  ix_t *sm = vec_add_vec(p,lm,1-p,bg);
  ix_t *cla = vec_x_vec (sm, 'H', bg);
  free_vec (lm); free_vec (sm); // don't free bg, it's static!
  return cla;
}

ix_t *qry_gain (ix_t *pos, ix_t *neg) {
  ix_t *all = vec_x_vec (pos, '|', neg); // union
  ix_t *qry = qry_centr (pos), *q;
  for (q = qry; q < qry+len(qry); ++q) {
    ix_t *inv = get_vec (INVL, q->i); // all docs containing q
    ix_t *occ = vec_x_vec (inv, '&', all); // project to union
    q->x = igain (occ, pos, 0);
    free_vec(inv); free_vec(occ);
  }
  free_vec (all);
  return qry;
}

ix_t *rnk_qry (ix_t *pos, char *prm) { // thread-unsafe: qry_diff qry_clarity
  uint qlen = getprm(prm,"len=",0);
  //strstr(prm,"gain") ? qry_gain (pos, neg) : 
  ix_t *qry = (strstr(prm,"clar") ? qry_clarity (pos, prm) :
	       strstr(prm,"logr") ? qry_logr (pos, prm) :
	       strstr(prm,"cent") ? qry_centr (pos) :
	       strstr(prm,"diff") ? qry_diff (pos) : NULL);
  if (qlen) trim_vec (qry, qlen);
  //if (strstr(prm,"gain")||strstr(prm,"clar")) vec_x_num (qry, '=', 1./len(qry));
  return qry;
}

void rnk_drag_qry (rnk_t *rnk, char *prm) {  // thread-unsafe: rnk_qry
  float thresh = getprm (prm, "thresh=", -Inf); 
  uint set, m = rnk_max_set (rnk);
  for (set = 0; set <= m; ++set) {
    if (set && !rnk_count ('r',rnk,set)) continue;
    ix_t *pos = examples_for_set (set, rnk, prm); 
    ix_t *qry = rnk_qry (pos, prm); 
    ix_t *ret = cols_x_vec (INVL, qry); 
    rnk_t *r, *rEnd = rnk+len(rnk);
    for (r = rnk; r < rEnd; ++r) {
      float sim = vec_get (ret, r->doc);
      if (sim > thresh && sim > r->sim) { r->sim = sim; r->set = set; }
    }
    free_vec (pos); free_vec (qry); free_vec (ret);
  } 
}

void rnk_drag (rnk_t *rnk, rnk_t *r, char *prm) { // thread-unsafe: rnk_drag_qry
  //printf ("%c>%d\n", (r->rel ? '+' : '-'), set);
  r->set = r->rel;
  r->sim = Inf;
  if      (strstr(prm,"1nn")) rnk_drag_1nn (rnk, r, r->rel, prm);
  else if (strstr(prm,"ann")) rnk_drag_ann (rnk, prm);
  else if (strstr(prm,"knn")) rnk_drag_ann (rnk, prm);
  else if (strstr(prm,"svm")) rnk_drag_svm (rnk, prm);
  else if (strstr(prm,"qry")) rnk_drag_qry (rnk, prm);
  rnk_show (rnk);
}

rnk_t *rnk_find_miss (rnk_t *rnk, uint set) {
  rnk_t *r = rnk-1, *end = rnk+len(rnk);
  //if (set<0) while (++r < end) if (r->rel != r->set) return r;
  while (++r < end) if ((r->rel == set) && (r->set != set)) return r;
  return NULL;
}

float rnk_single_drag (rnk_t *rnk, uint q, char *prm) { // thread-unsafe: rnk_drag
  printf ("\nQuery %d: %d rels\n", q, rnk_count('r',rnk,1));
  rnk_t *r = rnk_find_miss (rnk, 1); // r should be in set 1 but isn't
  rnk_drag (rnk,r,prm);
  return rnk_eval_multi (rnk,"F1");
}

float rnk_round_robin (rnk_t *rnk, uint q, char *prm) { // thread-unsafe: rnk_drag
  int set, m = rnk_max_set (rnk);
  uint rels = len(rnk)-rnk_count('r',rnk,0), sets = rnk_num_sets(rnk);
  int drags = getprm(prm,"drags=",0), minset = getprm(prm,"minset=",0);
  printf ("\nQuery %d %d rels in %d sets, %d drags:\n", q, rels, sets, drags);
  rnk_show (rnk);
  double sumF1 = 0, numF1 = 0;
  while (drags-- > 0) {
    for (set = m; set >= minset; --set) { // round-robin drags
      if (set && !rnk_count ('r',rnk,set)) continue; // skip empty sets
      rnk_t *r = rnk_find_miss (rnk, set); 
      if (!r) break;
      rnk_drag (rnk,r,prm); // drag doc into its list
      sumF1 += rnk_eval_multi(rnk,"F1"); ++numF1;
    }
  }
  return sumF1 / MAX(1,numF1); // average F1 across all drags
}

/*
uint rnk_recall_1st (rnk_t *rnk, char *prm) { // thread-unsafe: rnk_drag
  float goal = getprm (prm, "F1=", 0.9); // target performance
  uint pdrags = 0, ndrags = 0;
  while (rnk_eval(rnk,"F1") < goal) {
    rnk_t *r = rnk, *end = rnk+len(rnk);
    while (rnk_eval(rnk,"Recall") < goal) {
      while (r < end && (r->rel != 1 || r->set == 1)) ++r; // find relevant doc not in set 1
      if (r >= end) break;
      assert (r < end && r->rel == 1 && r->set == 0);
      rnk_drag (rnk, r, prm); // drag it to set 1
      ++pdrags;
    }
    r = rnk;
    while (rnk_eval(rnk,"Precision") < goal) {
      while (r < end && (r->rel == 1 || r->set != 1)) ++r; // find non-relevant doc in set 1
      if (r >= end) break;
      assert (r < end && r->rel == 0 && r->set == 1);
      rnk_drag (rnk, r, prm); // drag it to set 0
      ++ndrags;
    }
  }
  printf ("%.2f: %d+ %d-\n", rnk_eval(rnk,"F1"), pdrags, ndrags);
  return pdrags + ndrags;
}

uint rnk_inorder (rnk_t *rnk, char *prm) { // thread-unsafe: rnk_drag
  float goal = getprm (prm, "F1=", 0.9); // target performance
  uint pdrags = 0, ndrags = 0;
  while (rnk_eval(rnk,"F1") < goal) {
    rnk_t *r = rnk, *end = rnk+len(rnk);
    for (r = rnk; r < end; ++r) {
      if (r->rel == 1 && r->set != 1) { rnk_drag (rnk, r, prm); ++pdrags; break; }
      if (r->rel != 1 && r->set == 1) { rnk_drag (rnk, r, prm); ++ndrags; break; }
    }
  }
  printf ("%.2f: %d+ %d-\n", rnk_eval(rnk,"F1"), pdrags, ndrags);
  return pdrags + ndrags;
}
*/

void do_rnk (char *_RELS, char *_RETS, char *_DOCS, char *_INVL, char *_SIMS, char *prm) {
  char buf[999];
  RELS = open_coll (_RELS, "r+"); // relevance judgments
  RETS = open_coll (_RETS, "r+"); // old ranked lists
  DOCS = open_coll (_DOCS, "r+"); // document vectors
  INVL = open_coll (_INVL, "r+"); // inverted lists
  SIMS = open_coll (_SIMS, "r+"); // doc-to-doc similarities
  SIMT = open_coll (fmt(buf,"%s.T",_SIMS), "r+");
  uint q = 0, nq = num_rows (RETS);
  double avg = 0;
  for (q=1; q <= nq; ++q) {
    rnk_t *rnk = ixy_to_rnk (ranked_list (RETS,RELS,q), 300);
    //avg += log(rnk_recall_1st (rnk, prm));
    //avg += rnk_inorder (rnk, prm);
    avg += rnk_round_robin (rnk, q, prm);
    free_vec (rnk);
  }
  printf ("%.4f objective\n", avg/nq);
  free_coll (RELS);
  free_coll (RETS);
  free_coll (DOCS);
  free_coll (INVL);
  free_coll (SIMS);
  free_coll (SIMT);
}

//////////////////////////////////////////////////////////////// CFC weighting

// weight (t,j) = exp (DF_t,j / C_j * log b) * log (C / CF_t)
// DF_t,j ... # docs in category j that contain term t
// C_j    ... # docs in category j
// CF_t   ... # categories that contain term t
// C      ... # categories
// b=e-1  ... parameter > 1


//////////////////////////////////////////////////////////////// end test 3

void do_stats (char *_M, char *_H) {
  coll_t *M = open_coll (_M, "r+");
  hash_t *H = open_hash (_H, "r");
  stats_t *S = coll_stats (M);
  dump_stats (S, H);
  free_stats (S);
  free_hash (H);
  free_coll (M);
}

char *usage = 
  "kvs                           - optional [parameters] are in brackets\n"
  "  -m 256                      - set mmap size to 256MB\n"
  "  -rs 1                       - set random seed to 1\n"
  "  -dump XML [HASH id]         - dump all [id] from collection XML\n"
  "  -load XML HASH [prm]        - stdin -> collection XML indexed by HASH\n"
  "  -json JSON HASH [prm]       - stdin -> collection JSON indexed by HASH\n"
  "                                prm: skip, join duplicates, addkeys\n"
  //"  -merge C = A + B            - C[i] = A[i] + B[i] (concatenates records)\n"
  //"  -rekey A a = B b [addnew]   - A[j] = B[i] where key = a[j] = b[i]\n"
  //"   merge A += B [prm]         - A[j] += B[i] (concat, assume ids compatible)\n"
  "   merge A a += B b [prm]     - A[j] += B[i] (concat) where key = a[j] = b[i]\n"
  "                                prm: addnew ... add new keys if not in a\n"
  "  -stat XML HASH              - stats (cf,df) from collection XML -> stdout\n"
  "  -dmap XML HASH              - stdin: qryid docid, stdout: qryid XML[docid]\n"
  "  -qry 'query' DICT stem=L    - parse query\n"
  "  -ret  INVL [prm]            - retrieved set, prm ignored\n"
  "  -exp  DOCS [prm]            - Q = qw Q + top nw terms from top nd docs\n"
  "  -sort ORDER [asc]           - re-rank set by 1st vector in ORDER\n"
  "  -pair RANKS PAIRS           - save RANKS of relevant PAIRS of retrieved docs\n"
  "  -out  XML RNDR              - dump docs and render strings from XML\n"
  "  -mst OUT RETS DOCS [top=k]\n"
  "  -thr OUT RETS [prm]         - irank,ttest,rnk=K,num=K,min=K,pri=K\n"
  "  -test RELS RETS SIMS prm    - batch test:\n"
  "                                rel=1 ... drag 1 relevant doc into list\n"
  "                                non=0 ... drag 0 non-relevant docs away\n"
  "                                thr=0 ... follow only if sim > 0.0\n"
  "                                skp=0 ... skip a drag w. probability 0.0\n"
  "  -test2 RELS RETS SIMS prm\n"
  "  -rnk RELS RETS DOCS INVL SIMS prm:\n"
  "                                F1=0.9 ... keep dragging until F1 = 0.9\n"
  "                                sim=0  ... follow drag only if sim > 0\n"
  "                                top=K  ... drag only K nearest neighbours\n"
  ;

#define a(i) ((i < argc) ? argv[i] : "")

int main (int argc, char *argv[]) {
  char *QRY, *DICT, *INVL, *DOCS, *XML, *RNDR, *RELS, *RETS, *SIMS, *OUT, *ORDER, *PAIRS, *RANKS;
  if (argc < 3) return fprintf (stderr, "%s", usage); 
  ix_t *qry = NULL, *ret = NULL;
  vtime();
  while (++argv && --argc) {
    if (!strcmp (a(0), "-m")) MAP_SIZE = ((ulong) atoi (a(1))) << 20;
    if (!strcmp (a(0), "-rs")) srandom (atoi(a(1)));
    if (!strcmp (a(0), "-load")) load_raw (a(1), a(2), a(3));
    if (!strcmp (a(0), "-json")) load_json (a(1), a(2), a(3));
    //if (!strcmp (a(0), "-merge") &&
    //!strcmp (a(2), "="))     merge_colls (a(1), a(3), a(5));
    //if (!strcmp (a(0), "-rekey") &&
    //!strcmp (a(3), "="))     rekey_coll (a(1), a(2), a(4), a(5), a(6));
    if (!strcmp (a(0), "merge") && 
	!strcmp (a(3), "+="))    do_merge (a(1), a(2), a(4), a(5), a(6));
    if (!strcmp (a(0), "merge2") && 
	!strcmp (a(3), "+="))    do_merge2 (a(1), a(2), a(4), a(5), a(6));
    //if (!strcmp (a(0), "merge") && 
    //!strcmp (a(2), "+="))    do_merge (a(1), NULL, a(3), NULL, a(4));
    if (!strcmp (a(0), "-dump")) dump_raw (a(1), a(2), a(3));
    if (!strcmp (a(0), "-rand")) dump_rnd (a(1), a(2));
    if (!strcmp (a(0), "-dmap")) dump_raw_ret (a(1), a(2));
    if (!strcmp (a(0), "-stat")) do_stats (a(1), a(2));
    if (!strcmp (a(0), "-qry")) qry = do_qry (QRY=a(1), DICT=a(2), a(3));
    if (!strcmp (a(0), "-ret")) ret = do_ret (qry, INVL=a(1), a(2)); // free ret
    if (!strcmp (a(0), "-exp")) qry = do_exp (qry, ret, DOCS=a(1), a(2)); 
    if (!strcmp (a(0), "-sort")) do_sort (ret, ORDER=a(1), a(2)); 
    if (!strcmp (a(0), "-pair")) do_pairs (ret, RANKS=a(1), PAIRS=a(2)); 
    if (!strcmp (a(0), "-out")) do_out (ret, XML=a(1), RNDR=a(2));
    if (!strcmp (a(0), "-test")) do_test (RELS=a(1), RETS=a(2), SIMS=a(3), a(4));
    if (!strcmp (a(0), "-test2")) do_test2 (RELS=a(1), RETS=a(2), SIMS=a(3), a(4));
    if (!strcmp (a(0), "-mst")) do_mst (OUT=a(1), RETS=a(2), DOCS=a(3), a(4));
    if (!strcmp (a(0), "-thr")) do_thr (OUT=a(1), RETS=a(2), a(3));
    if (!strcmp (a(0), "-rnk")) do_rnk (a(1), a(2), a(3), a(4), a(5), a(6));
  }
  if (qry) free_vec (qry);
  if (ret) free_vec (ret);
  return 0;
}
