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

#include "matrix.h"

#ifndef QUERY
#define QUERY

// ------------------------------ index ------------------------------

typedef struct {
  hash_t *WORD;
  hash_t *DOC;
  coll_t *DOCxWORD;
  coll_t *WORDxDOC;
  coll_t *XML;
  coll_t *JSON;
  stats_t *STATS;
} index_t;

index_t *open_index (char *dir) ;
void free_index (index_t *I) ;

// ------------------------------ snippets ------------------------------

typedef struct {
  float score;
  char *id;
  char *url;
  char *head;
  char *snip;
  char *meta;
  char *full;
  uint date;
  uint group;
} snip_t;

// hack
int num_snippets(snip_t *S);
// de-allocates a vector of snippets.
void free_snippets (snip_t *S) ;
// returns full text as a snippet for each each doc in D.
snip_t *bare_snippets (ix_t *D, hash_t *IDs) ; // just id and score
snip_t *lazy_snippets (jix_t *D, coll_t *XML, hash_t *IDs) ;
// shrink snippet to size, rerank by how many words it covers.
void rerank_snippets (snip_t *S, char *qry, char **words, char *prm) ;
// trim docs -> lazy_snippets -> rerank_snippets (helper for run_*_qry)
snip_t *ranked_snippets (index_t *I, jix_t *docs, char *qry, char **toks, char *prm) ;
// compares two snippets by decreasing score.
int cmp_snip_score (const void *n1, const void *n2) ;

// returns a cleaned copy of XML[id], or NULL
char *copy_doc_text (coll_t *XML, uint id) ;
// ------------------------- query -------------------------

typedef struct {
  char *tok; // token behind query term
  char op; // &:and +:or -:not x:skip
  char type; // ~:synonyms ":exact *:edits <>=:threshold x:skip
  float thr; // threshold value
  uint id;   // id (once determined)
  uint id2;  // if token is a run-on
  ix_t *docs; // matching docs (once found)
  //void *children; // for nested queries
} qry_t;

// convert string to query operators and terms
qry_t *parse_qry (char *str, char *prm) ;
// de-allocate query
void free_qry (qry_t *Q) ;
// print internal form of query on stdout
void dump_qry (qry_t *Q) ;
// return a string form of the query (inverse of parse_qry)
char *qry2str (qry_t *Q, hash_t *H) ;
// like qry2str, but uses original terms, not spell-corrected
char *qry2str2 (qry_t *Q, hash_t *H) ;
// true if any of query terms were spell-corrected / stemmed etc
int qry_altered (qry_t *Q, hash_t *H) ;
// converts (structured) query to a list of tokens (for snippets)
char **qry2toks (qry_t *Q, hash_t *H) ;

// corrects spelling of every query word, fuses if helpful
void spell_qry (qry_t *Q, hash_t *H, float *F, char *prm) ;

// combine inv.lists for each term in Q using AND / OR / NOT
ix_t *exec_qry (index_t *I, qry_t *Q, char *prm) ;

snip_t *run_bool_qry (index_t *I, char *qry, char *prm) ;
snip_t *run_text_qry (index_t *I, char *qry, char *prm, char *mask) ;

snip_t *text_keywords (index_t *I, char *text, uint limit) ;
char **text_qry_toks (index_t *I, char *qry, char *prm) ;

ix_t *band_qry (ix_t *Q, coll_t *INVL) ;
ix_t *timed_qry (ix_t *_Q, coll_t *INVL, char *prm) ;

#endif
