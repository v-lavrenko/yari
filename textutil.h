/*
  
  Copyright (c) 1997-2016 Victor Lavrenko (v.lavrenko@gmail.com)
  
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

void noeol (char *str); // remove trailing newline
void chop (char *str, char *blanks) ; // remove head/tail blanks

char *substr (char *str, uint n) ; // str[0:n) in a static buffer
void reverse (char *str, uint n) ; // reverse sub-string of length n

uint atou (char *s) ; // string to integer (unsigned, decimal)
char *itoa (char*_a, uint i) ;


void squeeze (char *str, char *what) ;
void cgrams (char *str, uint lo, uint hi, uint step, char *buf, uint eob) ; // character n-grams

// erase (overwrite with 'C') every occurence of A...B
void erase_between (char *buf, char *A, char *B, int C) ;
char *json_value (char *json, char *key) ;
double json_numval (char *json, char *key) ;
char *json_pair (char *json, char *_str) ;

// extract the text between A and B
char *extract_between (char *buf, char *A, char *B) ;

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
void stem_toks (char **toks, char *type) ;
void stop_toks (char **toks) ;
ix_t *toks2ids (char **toks, hash_t *ids) ;
char **toks2pairs (char **toks, char *prm) ;
void free_toks (char **toks) ;

char *get_xml_docid (char *xml) ;

char *lowercase(char *s); // in-place
char *uppercase(char *s); // in-place
int cntchr (char *s, char c);

char *strRchr (char *beg, char *end, char key) ;
void purge_escaped (char *txt) ;

#endif
