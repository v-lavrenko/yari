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

#ifndef TEXTUTIL
#define TEXTUTIL

uint cntchr (char *s, char c) ; // count c in s
char *endchr (char *s, char c, uint n) ; // faster strrchr
// in str replace any occurence of chars from what[] with 'with'
void csub (char *str, char *what, char with) ; 
void cqsub (char *str, char what, char with, char quot) ;
// in haystack replace every instance of "needle" with 'with'
void gsub (char *haystack, char *needle, char with) ;
// in text replace a prefix of each "src" with "trg"
void psub (char *text, char *src, char *trg) ;

void noeol (char *str); // remove trailing newline
void chop (char *str, char *blanks) ; // remove head/tail blanks

char *substr (char *str, uint n) ; // str[0:n) in a malloc'd buffer
void reverse (char *str, uint n) ; // reverse sub-string of length n

char *strchr1 (char *s, char c) ; // point after 'c' in "s"
char *strstr1 (char *s, char *t) ; // point after 'c' in "s"

char *strrstr (char *s, char *t) ; // pointer to last t in s or NULL

uint atou (char *s) ; // string to integer (unsigned, decimal)
char *itoa (char*_a, uint i) ;

int alphanumeric (char *str) ;
void alphanumerize (char *str) ;

void squeeze (char *str, char *drop, char *keep) ;
void spaces2space (char *str) ; // multiple spaces -> single space
void cgrams (char *str, uint lo, uint hi, uint step, char *buf, uint eob) ; // character n-grams

// in S find+replace "&plusmn;" -> SRC -> id -> TRG -> "+/-"
void gsub_xml_refs (char *S, hash_t *SRC, coll_t *TRG) ; 
void no_xml_refs (char *S) ; // erase &quot; &amp; &#x10f; etc
void no_xml_tags (char *S) ; // erase <...>

// erase (overwrite with 'C') every occurence of A...B
void erase_between (char *buf, char *A, char *B, int C) ;
char *json_value (char *json, char *key) ;
double json_numval (char *json, char *key) ;
char *json_pair (char *json, char *_str) ;
char *json_docid (char *json) ;

void stop_at_chr (char *text, char *chr) ;
void stop_at_str (char *text, char *str) ;
// copy part of S before B
char *str_before (char *S, char *B) ;
// copy part of S after A
char *str_after (char *S, char *A) ;
// extract the text between A and B
char *extract_between (char *buf, char *A, char *B) ;
char *extract_between_nocase (char *buf, char *A, char *B) ;
int begins(char *S, char *T) ; // 1 if S begins with T

char *next_token (char **text, char *ws) ;

char *tsv_value (char *str, uint col) ;
char **split (char *str, char sep) ;
uint split2 (char *str, char sep, char **_tok, uint ntoks) ;
char **strsplit (char *str, char *sep) ;

char **readlines(char *path) ;
int read_doc (FILE *in, char *buf, int n, char *beg, char *end) ;

void porter_stemmer (char *word, char *stem) ;
void kstem_stemmer (char *word, char *stem) ;
void arabic_stemmer (char *word, char *stem) ;
void lowercase_stemmer (char *word, char *stem) ;
void stem_word (char *word, char *stem, char *type) ;
int stop_word (char *word) ;

char **str2toks (char *str, char *ws, uint maxlen) ;
char *toks2str (char **toks) ; // ' '.join(toks)
void stem_toks (char **toks, char *type) ;
void stop_toks (char **toks) ;
void keep_midsize_toks (char **toks, uint lo, uint hi) ;
void keep_wordlike_toks (char **toks) ;
char **text_to_toks (char *text, char *prm) ;

ix_t *toks2vec (char **toks, hash_t *ids) ;
char **vec2toks (ix_t *vec, hash_t *ids) ;
char **toks2pairs (char **toks, char *prm) ;
void free_toks (char **toks) ;
void show_toks (char **toks, char *fmt) ;

char *get_xml_docid (char *xml) ;
char *get_xml_title (char *str) ;
char *get_xml_author (char *xml) ;
char *get_xml_open (char *xml, char *tag); // pointer to next opening <tag>
char *get_xml_close (char *xml, char *tag); // pointer to next closing </tag>
char *get_xml_intag (char *xml, char *tag); // string between <tag> and </tag>
char *get_xml_inpath (char *xml, char *path); // string in "body.ref.id"
char *get_xml_intags (char *xml, char **tags); // string in ["body","ref","id"]
char *get_xml_all_intag (char *xml, char *tag, char sep) ; // all <tag>...</tag>

void append_json (char **trg, size_t *sz, char *src) ; // {trg},{src} -> {trg, src}
void append_sgml (char **trg, size_t *sz, char *src) ; // <TRG>,<SRC> -> <TRG SRC>

char *lowercase(char *s); // in-place
char *uppercase(char *s); // in-place

char *strRchr (char *beg, char *end, char key) ;
void purge_escaped (char *txt) ;

ijk_t *hits_for_all_words (char *_text, char **words) ;
ijk_t *hits_for_xml_tags (char *S) ;
ijk_t *hits_with_prefix (char *S, ijk_t *H, char *prefix) ;

int nearest_ws (char *text, int position, int cap) ;
char *snippet2 (char *text, char **words, int sz, float *score);
char *hl_snippet (char *text, char **words, int sz, float *score) ;
char *curses_snippet (char *text, char **words, int sz, float *score) ;
char *html_snippet (char *text, char **words, int sz, float *score) ;

void show_tokens (sjk_t *T, char *tag) ;
void free_tokens (sjk_t *T) ;
void stop_tokens (sjk_t *T) ;
sjk_t *go_tokens (char *text) ;
int skip_token(sjk_t *t) ;
sjk_t *go_unigrams (sjk_t *T) ;
sjk_t *go_bigrams (sjk_t *T) ;
sjk_t *go_ngrams (sjk_t *T, uint n) ;
hash_t *ngrams_dict (char *qry, int n) ;
ijk_t *ngram_matches (sjk_t *tokens, hash_t *H, int n) ;
ijk_t *merge_matches (ijk_t *M) ;
uint total_ijk_len (ijk_t *M) ;
uint total_sjk_len (sjk_t *M) ;
ijk_t best_ngram_span (ijk_t *M, uint sz, uint textLen) ;
char *ngram_snippet (char *text, hash_t *Q, int gramsz, int snipsz, float *score) ;
char *hybrid_snippet (char *text, char **words, hash_t *Q, int gramsz, int snipsz, float *score) ;
char *ngram_highlight (char *text, hash_t *Q, int gramsz) ;

float pack_span (uint beg, uint end); // packs span into a single float (lossy)
it_t unpack_span (float x); // unpacks span (lossy)

#endif
