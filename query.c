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
#include "hl.h"
#include "math.h"
#include "cluster.h"

// ------------------------------ index ------------------------------

index_t *open_index (char *dir) {
  char _[999];
  index_t *I = calloc (1, sizeof(index_t));
  I->DOC      = open_hash_if_exists (fmt(_,"%s/DOC",dir), "r");
  I->WORD     = open_hash_if_exists (fmt(_,"%s/WORD",dir), "r");
  I->WORDxDOC = open_coll_if_exists (fmt(_,"%s/WORDxDOC",dir), "r+");
  I->DOCxWORD = open_coll_if_exists (fmt(_,"%s/DOCxWORD",dir), "r+");
  I->JSON     = open_coll_if_exists (fmt(_,"%s/JSON",dir), "r");
  I->XML      = open_coll_if_exists (fmt(_,"%s/XML",dir), "r");
  I->STATS    = open_stats_if_exists (fmt(_,"%s/STATS",dir));
  return I;
}

void free_index (index_t *I) {
  if (I->DOC) free_hash (I->DOC);
  if (I->WORD) free_hash (I->WORD);
  if (I->WORDxDOC) free_coll (I->WORDxDOC);
  if (I->DOCxWORD) free_coll (I->DOCxWORD);
  if (I->JSON) free_coll (I->JSON);
  if (I->XML) free_coll (I->XML);
  if (I->STATS) free_stats (I->STATS);
  memset (I, 0, sizeof(index_t));
  free (I);
}

// ------------------------------ snippets ------------------------------

// workaround for len(S) not working outside Yari
int num_snippets(snip_t *S) {return len(S);}

// de-allocates a vector of snippets.
void free_snippets (snip_t *S) {
  snip_t *s, *sEnd = S + len(S);
  for (s = S; s < sEnd; ++s) {
    if (s->id) free (s->id);
    if (s->url)  free (s->url);
    if (s->head) free (s->head);
    if (s->snip) free (s->snip);
    if (s->meta) free (s->meta);
    if (s->full) free (s->full);
  }
  memset (S, 0, len(S) * sizeof(snip_t));
  free_vec (S);
}

// returns a cleaned copy of XML[id], or NULL
char *copy_doc_text (coll_t *XML, uint id) {
  char *chunk = id ? get_chunk(XML,id) : NULL;
  if (!chunk) return NULL;
  char *text = strdup(chunk);
  erase_between(text, "<DOCID>", "</DOCID>", ' ');
  //char *text = extract_between (chunk, "</DOCID>", "</DOC>");
  erase_between(text, "<title", ">", '\t');
  erase_between(text, "<p:",    ">", '\t');
  erase_between(text, "<claim", ">", '\t');
  gsub (text, "</title>", '\t');
  gsub (text, "</claim>", '\t');
  gsub (text, "</p>",     '\t');
  no_xml_tags (text);
  chop (text, " "); // chop whitespace around docid
  //no_xml_refs (text);
  csub (text, "\"\\\r\n", ' ');
  //fprintf(stderr,"copy_doc_text: %d -> len:%ld CR:%d\n", id, strlen(text), cntchr(text,'\r'));
  return text;
}

// returns just id and score for each doc in D.
snip_t *bare_snippets (ix_t *D, hash_t *IDs) {
  uint i, n = len(D);
  snip_t *S = new_vec(n, sizeof(snip_t));
  for (i = 0; i < n; ++i) {
    S[i].score = D[i].x;
    S[i].id = id2str (IDs, D[i].i);
  }
  return S;
}

// returns full text as a snippet for each each doc in D.
snip_t *lazy_snippets (jix_t *D, coll_t *XML, hash_t *IDs) {
  uint i, n = len(D);
  snip_t *S = new_vec(n, sizeof(snip_t));
  for (i = 0; i < n; ++i) {
    S[i].snip = copy_doc_text (XML, D[i].i); // clean copy
    S[i].score = D[i].x;
    S[i].id = id2str (IDs, D[i].i);
    S[i].group = D[i].j;
  }
  return S;
}

// shrink snippet to size, rerank by how many words it covers.
void rerank_snippets (snip_t *S, char *qry, char **words, char *prm) {
  //printf ("%squery%s: %s %s\n", fg_RED, RESET, qry, prm);
  uint qryLen = strlen(qry);
  uint snipsz = getprm(prm,"snipsz=",0);
  if (!snipsz) return sort_vec (S, cmp_snip_score);
  char *hilit = strstr(prm,"hilit");
  char *curses = strstr(prm,"curses");
  char *hihtml = strstr(prm,"hihtml");
  char *paragr = strstr(prm,"paragr");
  uint ngramsz = getprm(prm,"ngramsz=",0);
  //char **words = text_to_toks (qry, prm); // 3/18
  hash_t *Q = ngramsz ? ngrams_dict (qry, ngramsz) : NULL;
  snip_t *s;
  for (s = S; s < S + len(S); ++s) {
    char *old = s->snip;
    float score = s->score;
    //printf ("%srerank%s: %s %s\n", fg_RED, RESET, s->id, s->snip);
    //if     (ngramsz) s->snip = hybrid_snippet (old, words, Q, ngramsz, snipsz, &score);
    //if     (ngramsz) s->snip = (len(words) > 3 ?
    //ngram_snippet (old, Q, ngramsz, snipsz, &score) :
    //html_snippet (old, words, snipsz, &score));
    if     (ngramsz) s->snip = (paragr ?
				best_paragraph (old, qry, Q, ngramsz, &score) : 
				qryLen > 60 ?
				ngram_snippet (old, Q, ngramsz, snipsz, &score) :
				html_snippet (old, words, snipsz, &score));
    else if (curses) s->snip = curses_snippet (old, words, snipsz, &score);
    else if (hihtml) s->snip = html_snippet (old, words, snipsz, &score);
    else if  (hilit) s->snip = hl_snippet (old, words, snipsz, &score);
    else             s->snip = snippet2 (old, words, snipsz, &score);
    s->score = score;
    free(old);
    //break; // FIXME!
  }
  sort_vec (S, cmp_snip_score);
  free_hash (Q);
  //free_toks (words); // 3/18
}

// helper function: extracts & reranks snippets for docs
snip_t *ranked_snippets (index_t *I, jix_t *docs, char *qry, char **toks, char *prm) {
  uint rerank = getprm(prm,"rerank=",50);
  sort_vec (docs, cmp_jix_X);
  if (len(docs) > rerank) len(docs) = rerank;
  snip_t *S = lazy_snippets (docs, I->XML, I->DOC);
  loglag("lazy");
  rerank_snippets (S, qry, toks, prm);
  loglag("rerank");
  return S;
}

// compares two snippets by decreasing score.
int cmp_snip_score (const void *n1, const void *n2) {
  float x1 = ((snip_t*)n1)->score;
  float x2 = ((snip_t*)n2)->score;
  return (x1 > x2) ? -1 : (x1 < x2) ? +1 : 0;
}

// ------------------------- novel / diverse -------------------------

// compare D[i] to D[0:i-1], drop if cosine > dedup
void dedup_docs (ix_t *D, coll_t *DOCS, char *prm) {
  sort_vec(D, cmp_ix_X);
  uint rerank = getprm(prm,"rerank=",50);
  float dedup = getprm(prm, "dedup=", 1.1);
  if (dedup > 1 || !D) return; // nothing to do
  ix_t *d, **U = new_vec (0, sizeof(ix_t*)), **u;
  for (d = D; d < D+len(D); ++d) {
    ix_t *doc = get_vec_ro (DOCS, d->i);
    for (u = U; u < U+len(U); ++u) // look for duplicates
      if (cosine(doc,*u) > dedup) break; // found d ~ *u
    if (u < U+len(U)) d->i = 0;
    else { // d is novel => add it to used set U
      doc = copy_vec(doc);
      U = append_vec(U, &doc);
    }
    if (len(U) > rerank) break;
  }
  for (u = U; u < U+len(U); ++u) free_vec(*u);
  free_vec (U);
  len(D) = d-D;
  chop_vec(D);
}


// ------------------------- keywords -------------------------

// return docs that follow docs in D
ix_t *future_docs (ix_t *D, uint maxId, char *prm) {
  uint ni = getprm(prm,"future=",0), i;
  if (!ni) return copy_vec(D);
  float p = getprm(prm,"decay=",0.5);
  ix_t *F = new_vec (0, sizeof(ix_t)), *d;
  for (d = D; d < D+len(D); ++d)
    for (i = 0; i < ni; ++i) {
      uint id = MIN(maxId, (d->i + i+1)); // assume docs sequential
      float x = d->x + i * log(p); // assume softmax
      F = append_vec(F, & (ix_t) {id,x});
    }
  return F;
}

ix_t *rm_words (ix_t *Q, coll_t *DxW, coll_t *WxD, char *prm) {
  uint limit = getprm(prm,"limit=",50);
  uint scale = getprm(prm,"scale=",1.0);
  ix_t *D0 = timed_qry (Q, WxD, prm); // deadline=100ms,beam=10000
  trim_vec (D0, limit);
  //ix_t *D1 = future_docs(D0, num_rows(DxW), prm); // future=1,decay=0.5
  vec_x_num (D0, '*', scale);
  softmax (D0);
  ix_t *RM = cols_x_vec (DxW, D0);
  free_vec (D0);
  //free_vec (D1);
  return RM;
}

/*
double do_eval_rm (coll_t *DxW, char *prm) {
    char *docid = getprms(prm,"docid=",NULL,",");
  uint id = key2id(I->DOC,docid);
  char *text = copy_doc_text (I->XML, id);

  return lnP_X_Y (ix_t *X, ix_t *Y, double mu, float *CF, ulong nposts)  / sum(X);
}
*/


// ------------------------- BM25 keywords -------------------------

ix_t *bm25_keywords (char *text, hash_t *H, stats_t *S) {
  char **words = text_to_toks (text, "stop,stem=L");
  ix_t *vec = toks2vec (words, H);
  sort_uniq_vec (vec);
  weigh_mtx_or_vec (0, vec, "idf", S);
  free_toks(words);
  return vec;
}

void show_vec (ix_t *vec, char what) ;

snip_t *text_keywords (index_t *I, char *text, uint limit) {
  ix_t *W = bm25_keywords (text, I->WORD, I->STATS), *w;
  sort_vec (W, cmp_ix_X);
  //show_vec (W,'*');
  if (limit && (len(W) > limit)) len(W) = limit;
  snip_t *S = new_vec (len(W), sizeof(snip_t));
  for (w=W; w < W+len(W); ++w) {
    snip_t new = {w->x, 0, 0, 0, id2str(I->WORD,w->i), 0, 0, 0, 0};
    S[w-W] = new;
  }
  free_vec (W);
  return S;
}

// ------------------------- structured query -------------------------

// converts (structured) query to a list of tokens (for snippets)
char **qry2toks (qry_t *Q, hash_t *H) {
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

// convert string to query operators and terms
qry_t *parse_qry (char *str, char *prm) {
  char *stem = getprmp (prm, "stem:", "L");
  char *stop = strstr (prm, "stop");
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
    if (!inquote && !*thr && *stem == 'K') kstem_stemmer (s,s);
    q->tok = s; // strdup(s)?
    q->op = inquote ? qop : op;
    if (stop && stop_word (s)) q->op = 'x'; // skip stop words
    q->type = q->type ? q->type : inquote ? inquote : '.';
    if ( inquote && (*last == '\'')) *last = inquote = 0; // end quote
  }
  free_vec(toks);
  return Q;
}

// de-allocate query
void free_qry (qry_t *Q) {
  qry_t *q;
  for (q = Q; q < Q+len(Q); ++q) {
    if (q->docs) free_vec (q->docs);
    //if (q->children) free_qry (q->children);
    //if (q->tok) free (q->tok); // do not free: q->tok points inside buffer
  }
  free_vec (Q);
}

// print internal form of query on stdout
void dump_qry (qry_t *Q) {
  qry_t *q = Q-1, *z = Q + len(Q);
  while (++q<z) printf ("%c\t%c\t%s\t%.2f\n", q->op, q->type, q->tok, q->thr);
}

// return a string form of the query (inverse of parse_qry)
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

// like qry2str, but uses original terms, not spell-corrected
char *qry2str2 (qry_t *Q, hash_t *H) {
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

// true if any of query terms were spell-corrected / stemmed etc
int qry_altered (qry_t *Q, hash_t *H) {
  qry_t *q, *last = Q+len(Q)-1;
  for (q = Q; q <= last; ++q) {
    char *term = q->id ? id2key(H,q->id) : NULL;
    if (term && strcmp(term,q->tok)) return 1;
  }
  return 0;
}

// ------------------------- spell-check -------------------------

// corrects spelling of every query word, fuses if helpful
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

// ------------------------- execute qry -------------------------

// combine inv.lists for each term in qry using AND / OR / NOT
ix_t *exec_qry (index_t *I, qry_t *Q, char *prm) {
  char *verbose = strstr(prm,"verbose");
  double T = mstime();
  qry_t *q; ix_t *result = NULL;
  for (q = Q; q < Q+len(Q); ++q) {
    if (q->op == 'x') continue; // skip this term
    if (!q->id) q->id = has_key (I->WORD, q->tok);
    q->docs = get_vec (I->WORDxDOC, q->id);
    //if (q->children) q->docs = exec_qry (I, q->children)
    if (q->id2) { // term was a runon => intersect (L,R)
      ix_t *tmp = get_vec_ro (I->WORDxDOC, q->id2);
      filter_and_sum (q->docs, tmp);
    }
    if (!result) result = copy_vec (q->docs); // 1st term cannot be a negation
    else if (q->op == '.') filter_and_sum (result, q->docs);
    else if (q->op == '-') filter_not (result, q->docs);
    else if (q->op == '+') filter_sum (&result, q->docs);
    if (verbose) printf ("\t%.0fms %c%s -> [%d] -> [%d]\n",
			 msdiff(&T), q->op, id2key(I->WORD,q->id), len(q->docs), len(result));
  }
  return result ? result : new_vec(0,sizeof(ix_t));
}

// pqrse qry, execute, extract & rerank snippets
snip_t *run_bool_qry (index_t *I, char *qry, char *prm) {
  qry_t *Q = parse_qry (qry, prm); // prm: stop,stem=L
  //char *spell = strstr(prm,"spell");
  //if (spell) spell_qry (Q, I->WORD, I->STATS->cf, prm); // prm: pubmed_spell
  ix_t *docs = exec_qry (I, Q, prm); // prm: verbose
  jix_t *groups = ix2jix (0, docs); // no grouping
  char **toks = qry2toks (Q, I->WORD);
  snip_t *snips = ranked_snippets (I, groups, qry, toks, prm); // rerank=50, snipsz=300
  free_qry(Q);
  free_vec(docs);
  free_vec(groups);
  free_toks(toks);
  return snips;
}

snip_t *run_text_qry (index_t *I, char *_qry, char *prm, char *mask) {
  //fprintf(stderr,"\nrun_text_qry: '%s'\n", qry);
  if (!_qry) return new_vec (0, sizeof(snip_t));
  char *qry = strdup(_qry); // parse_vec destroys qry
  coll_t *INVL = I->WORDxDOC;
  ix_t *D, *Q = parse_vec_txt (qry, 0, I->WORD, prm); // stop,stem=K,tokw,nowb,gram=2:3
  jix_t *G = NULL; // groups of docs
  loglag("parse");
  //weigh_mtx_or_vec (0, Q, "idf,top=10", I->STATS); // inq,idf,top=10,thr=0,L2=1
  if      (strstr(prm,"band"))  D = band_qry (Q, INVL);  // Boolean AND (fast!)
  else if (strstr(prm,"timed")) D = timed_qry (Q, INVL, prm); // deadline=100ms,beam=10000
  else if (strstr(prm,"merge")) D = vec_x_rows (Q, INVL); // merge lists: few rare terms
  else if (strstr(prm,"score")) D = cols_x_vec (INVL, Q); // SCORE: many common terms
  else if (strstr(prm,"iseen")) D = cols_x_vec_iseen (INVL, Q);
  else if (strstr(prm,"iskip")) D = cols_x_vec_iskip (INVL, Q);
  else assert(0 && "must specify band | timed | merge | score | iseen | iskip");
  loglag("exec");
  if (mask) {
    vec_x_set(D, '*', mask);
    loglag("mask");
  }
  if (strstr(prm,"dedup")) {
    dedup_docs(D, I->DOCxWORD, prm);
    loglag("dedup");
  }
  if (strstr(prm,"clump")) {
    G = clump_docs (D, I->DOCxWORD, prm);
    //show_jix (G, "clumps", 'i');
    one_per_clump (G);
    //show_jix (G, "1/group", '*');
    loglag("clump");
  }
  else G = ix2jix (1, D); // no grouping / clumping
  char **toks = vec2toks (Q, I->WORD);
  snip_t *snips = ranked_snippets (I, G, _qry, toks, prm); // rerank=50, snipsz=300
  loglag("snippets");
  free_vec(Q);
  free_vec(D);
  free_vec(G);
  free_toks(toks);
  free(qry);
  return snips;
}

/* unused? (was used in do_find)
// returns toks as used by run_text_qry()
char **text_qry_toks (index_t *I, char *qry, char *prm) {
  qry = strdup(qry); // parse_vec destroys qry
  ix_t *Q = parse_vec_txt (qry, 0, I->WORD, prm); // stop,stem=K,tokw,nowb,gram=2:3
  weigh_mtx_or_vec (0, Q, "idf", I->STATS);
  sort_vec(Q, cmp_ix_X);
  if (len(Q) > 5) len(Q) = 5;
  char **toks = vec2toks (Q, I->WORD);
  free_vec(Q);
  free(qry);
  return toks;
}
*/

// Boolean AND of query terms (fast!)
ix_t *band_qry (ix_t *Q, coll_t *INVL) {
  ix_t *R = get_vec (INVL, Q->i), *q, *end = Q+len(Q);
  for (q = Q; q < end; ++q) {
    ix_t *D = get_vec_ro (INVL, q->i);
    filter_and_sum (R, D);
  }
  return R;
}

// cost of each query term, slowest last
ixy_t *qry_costs (ix_t *Q, coll_t *INVL) {
  ixy_t *C = ix2ixy(Q, 0), *c;
  for (c = C; c < C+len(C); ++c)
    c->y = (float) chunk_sz (INVL, c->i);
  sort_vec (C, cmp_ixy_y); // ascending y: invl size
  return C;
}

// match as many query terms as possible before deadline
// use DOT product, assume weighting pre-applied
ix_t *timed_qry (ix_t *_Q, coll_t *INVL, char *prm) {
  double deadline = mstime() + getprm (prm,"timed=",100);
  uint beam = getprm (prm, "beam=", 10000);
  ixy_t *Q = qry_costs (_Q, INVL), *q=Q, *end = Q + len(Q);
  if (q->y > 1e8) { free_vec(Q); return const_vec(0,0); }
  ix_t *R = get_vec (INVL, q->i);
  while (++q < end && mstime() < deadline) {
    ix_t *D = get_vec_ro (INVL, q->i), *tmp;
    R = vec_add_vec (1, tmp=R, q->x, D);
    free_vec(tmp);
    if (len(R) < beam) continue;
    qselect(R, beam);
    len(R) = beam;
    //assert (vec_is_sorted(R));
    sort_vec (R, cmp_ix_i);
  }
  free_vec(Q);
  return R;
}

#ifdef MAIN

int dump_parsed_qry (char *qry) {
  qry_t *Q = parse_qry (qry, ""), *q;
  //fprintf(stderr, "input: %d\n", len(Q));
  for (q = Q; q < Q + len(Q); ++q)
    printf ("%c\t%c\t%s\t%.2f\n", q->op, q->type, q->tok, q->thr);
  free_qry(Q);
  return 0;
}

int dump_spelled_qry (char *qry, char *_H, char *_F, char *prm) {
  qry_t *Q = parse_qry (qry, "");
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

int do_exec_qry (char *qry, char *index, char *prm) {
  index_t *I = open_index (index);
  snip_t *s, *S = run_bool_qry (I, qry, prm);
  for (s=S; s < S+len(S); ++s)
    printf ("%6.4f %s\n", s->score, s->snip);
  free_snippets(S);
  free_index(I);
  return 0;
}

// generate a fake query (string) from a collection of docs
char *fake_qry (index_t *I, char *prm) {
  char *docid = getprms(prm,"docid=",NULL,",");
  fprintf(stderr,"%sQuery: %s%s%s", fg_CYAN, fg_BLUE, docid, RESET);
  uint id = key2id(I->DOC,docid);
  char *text = copy_doc_text (I->XML, id);
  fprintf(stderr," %s\n", text);
  if (strstr(prm,"next")) {
    char *doc = copy_doc_text (I->XML, id+1);
    fprintf(stderr, "%sSuffix: %s%s\n", fg_CYAN, RESET, doc);
    free(doc);
  }
  fprintf(stderr,"\n");
  return text;
}

// show a diff of strings A & B on stdout.
void diff_A_B (char *A, char *B) {
  write_file ("/tmp/qdiff.A", A);
  write_file ("/tmp/qdiff.B", B);
  fflush(stdout);
  system("wdiff /tmp/qdiff.A /tmp/qdiff.B | colordiff");
  fflush(stdout);
}

void print_next_doc(index_t *I, char *docid) {
  uint id = key2id(I->DOC, docid);
  char *doc = copy_doc_text (I->XML, id+1);
  csub(doc, "0123456789",'n');
  printf("%s|%s%s", fg_MAGENTA, RESET, doc);
  free(doc);
}

int do_text_qry (char *qry, char *index, char *prm) {
  uint limit = getprm(prm,"limit=",10);
  char *diff = strstr(prm,"diff");
  char *next = strstr(prm,"next");
  index_t *I = open_index (index);
  if (strstr(qry,"docid=")) qry = fake_qry(I,qry);
  snip_t *s, *S = run_text_qry (I, qry, prm, NULL);
  limit = MIN(limit,len(S));
  for (s=S; s < S+limit; ++s) {
    if (diff) {
      printf ("%s%ld.%s %s %.2f %s%s ", fg_CYAN, s-S+1, RESET, fg_BLUE, s->score, s->id, RESET);
      diff_A_B (qry, s->snip);
      printf ("\n\n");
    }
    else if (next) {
      print_next_doc(I, s->id);
      printf("\n");
    }
    else printf ("%.2f %s %s\n", s->score, s->id, s->snip);
  }
  free_snippets(S);
  free_index(I);
  return 0;
}

int look_for_reuse (char *index, char *prm) {
  index_t *I = open_index (index);
  uint Len = getprm(prm,"len=",30);
  uint run = getprm(prm,"run=",10);
  float sim = getprm(prm,"sim=",0.95);
  uint nd = num_rows(I->DOCxWORD);
  coll_t *VECS = open_coll_if_exists ("DOCxWORD.tf", "r+");
  while (1) {
    uint id = 1 + (random() % nd);
    ix_t *Q = get_vec (VECS, id);
    if (sum(Q) < Len) {free_vec(Q); continue;} // too short
    ix_t *D = timed_qry (Q, I->WORDxDOC, prm), *d;
    sort_vec(D, cmp_ix_X);
    for (d = D; d < D+len(D); ++d)
      if ((d->x / D->x) < sim) break;
    if (d-D > run) {
      printf("%ld dups %s len: %.0f sim: %.2f %.2f ... %.2f\n",
	     d-D, id2key(I->DOC, id), sum(Q), D->x, (D+1)->x, (d-1)->x);
      fflush (stdout);
    }
    free_vec(Q);
    free_vec(D);
  }
  free_index(I);
  free_coll(VECS);
  return 0;
}


#define arg(i) ((i < argc) ? argv[i] : NULL)
#define a(i) ((i < argc) ? argv[i] : "")

char *usage =
  "usage: query -parse 'rimantadin acid ic +\"aspirine\"'\n"
  "       query -spell 'rimantadin acid ic' WORD WORD_CF [verbose]\n"
  "       query -exec  'rimantadin acid ic' dir [prm]\n"
  "       query -reuse dir [prm]\n"
  "       query -text  'rimantadin acid ic' dir [prm]\n"
  "              dir: DOC WORD DOCxWORD WORDxDOC XML STATS\n"
  "              qry: 'query words' | 'docid=X' \n"
  "              prm: stop,stem=L,tokw,gram=2:3,ow=2,uw=3\n"
  "                   band | merge | score | iseen | iskip | timed=100ms,beam=999\n"
  "                   dedup=0.9,rerank=50,limit=5,snipsz=100\n"
  "                   hilit | hihtml | curses | qdiff | ngram=3\n"
  ;

int main (int argc, char *argv[]) {
  if (argc < 2) return fprintf (stderr, "%s", usage);
  if (!strcmp(a(1),"-parse")) return dump_parsed_qry(a(2));
  if (!strcmp(a(1),"-spell")) return dump_spelled_qry(a(2), a(3), a(4), a(5));
  if (!strcmp(a(1),"-exec")) return do_exec_qry(a(2), a(3), a(4));
  if (!strcmp(a(1),"-text")) return do_text_qry(a(2), a(3), a(4));
  if (!strcmp(a(1),"-reuse")) return look_for_reuse (a(2), a(3));
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
