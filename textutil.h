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

char *endchr (char *s, char c, uint n) ; // faster strrchr
// in str replace any occurence of chars from what[] with 'with'
void csub (char *str, char *what, char with) ; 
void cqsub (char *str, char what, char with, char quot) ;
// in haystack replace every instance of "needle" with 'with'
void gsub (char *haystack, char *needle, char with) ;

void noeol (char *str); // remove trailing newline
void chop (char *str, char *blanks) ; // remove head/tail blanks

char *substr (char *str, uint n) ; // str[0:n) in a static buffer
void reverse (char *str, uint n) ; // reverse sub-string of length n

uint atou (char *s) ; // string to integer (unsigned, decimal)
char *itoa (char*_a, uint i) ;

void squeeze (char *str, char *what) ;
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

// extract the text between A and B
char *extract_between (char *buf, char *A, char *B) ;
char *extract_between_nocase (char *buf, char *A, char *B) ;

char *next_token (char **text, char *ws) ;

char *tsv_value (char *str, uint col) ; // 
char **split (char *str, char sep) ;
uint split2 (char *str, char sep, char **_tok, uint ntoks) ;

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
ix_t *toks2vec (char **toks, hash_t *ids) ;
char **vec2toks (ix_t *vec, hash_t *ids) ;
char **toks2pairs (char **toks, char *prm) ;
void free_toks (char **toks) ;

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
int cntchr (char *s, char c);

char *strRchr (char *beg, char *end, char key) ;
void purge_escaped (char *txt) ;

int nearest_ws (char *text, int position, int cap) ;
char *snippet2 (char *text, char **words, int sz);

#endif
