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

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "hash.h"
#include "textutil.h"

char *default_ws = " \t\r\n~`!@#$%^&*()_-+=[]{}|\\:;\"'<>,.?/";

char *endchr (char *s, char c, uint n) { // faster strrchr
  char *p = s + n - 1; // end of string
  while (p > s && *p != c) --p;
  return p > s ? p : NULL;
}

// in str replace any occurence of chars from what[] with 'with'
void csub (char *str, char *what, char with) {
  if (!what) what = default_ws;
  char *s, *t;
  for (s = str; *s; ++s) if (strchr (what, *s)) *s = with;
  for (t=s=str+1; *s; ++s) if (s[0] != with || s[-1] != with) *t++ = *s;
  *t = 0;
  //  char *s = str-1, *t = str, first = 0;
  //  while (*++s) 
  //    if (!strchr (what, *s)) {*t++ = *s; first = 1;}
  //    else if (first) {*t++ = with; first = 0; }
  //  *t = 0;
}

// change any occurence of 'what' to 'with' inside quotes
void cqsub (char *str, char what, char with, char quot) {
  char in_quot = 0;
  while (*str) {
    if (*str == what && in_quot) *str = with;
    if (*str == quot) in_quot = !in_quot;
    ++str;
  }
}

// in str replace any occurence of chars from punctuation[] with nothing
void squeeze (char *str, char *what) {
  if (!what) what = default_ws;
  char *s, *t; 
  for (t=s=str; *s; ++s) if (!strchr (what, *s)) *t++ = *s;
  *t = 0; // new string may be shorter
}

void cgrams (char *str, uint lo, uint hi, uint step, char *buf, uint eob) {
  uint n;
  char *t, *s, *end = str + strlen(str) - 1, *b = buf;
  while (str < end && *str == '_') ++str; // skip starting blanks
  while (end > str && *end == '_') --end; // skip ending blanks
  for (s = str; s+lo <= end+1; s+=step)
    for (n = lo; n <= hi; n+=step) {
      if (b+n+1 >= buf+eob) break;
      for (t = s; t < s+n; ++t) *b++ = *t;
      *b++ = ' ';
    }
  *b = 0;
}

uint split (char *str, char sep, char **_tok, uint ntoks) {
  char **tok = _tok, **last = _tok + ntoks - 1;
  *tok++ = str;
  for (; *str; ++str) if (*str == sep) { 
      *str++ = 0; 
      *tok++ = str; 
      if (tok > last) break;
    }
  return tok - _tok;
}

void chop (char *str, char *blanks) {
  char *beg, *end;
  unsigned len;
  if (!(str && blanks && *blanks)) return;
  beg = str + strspn (str, blanks); // skip initial blanks
  len = strlen (beg);
  memmove (str, beg, len); // slide right
  str [len] = '\0'; // null-terminate
  for (end = str + len - 1; end >= str && strchr (blanks, *end); --end) 
    *end = '\0'; // zero out trailing blanks
}

char *substr (char *s, uint n) {
  char *buf = NULL; assert (n < 999);
  if (!buf) buf = malloc(999);
  strncpy (buf, s, n);
  buf[n] = 0;
  return buf;
}

// erase (overwrite with 'C') every occurence of A...B
void erase_between (char *buf, char *A, char *B, int C) {
  int lenA = strlen (A);
  int lenB = strlen (B);
  char *a = buf, *b = buf;
  while ((a = strstr (a, A))) {
    if (!(b = strstr (a + lenA, B))) {
      fprintf (stderr, "[erase_between] open '%s' not closed by '%s'\n", A, B);
      return;
    }
    memset (a, C, b + lenB - a);
    a = b + lenB;
  }
}

// extract the text between A and B
char *extract_between (char *buf, char *A, char *B){
  char *a, *b, *result = NULL;
  if (!(a = strstr (buf, A))) return 0; // A not found
  a += strlen (A);     // skip A itself
  if (!(b = strstr (a, B))) {
    fprintf (stderr, "[extract_between] open '%s' not closed by '%s'\n", A, B);
    return 0;
  }
  result = calloc (b - a + 1, sizeof (char));
  strncpy (result, a, b - a);
  return result;
}

char *parenthesized (char *str, char open, char close) {
  int depth = 0; char *beg = strchr(str,open), *s; 
  if (!beg) return NULL; // no opening paren
  for (s = beg; *s; ++s) 
    if      (*s == close && --depth == 0) break;
    else if (*s == open)    ++depth;
  if (!*s) return NULL; // no closing paren
  return strndup (beg, s-beg+1);
}

uint parenspn (char *str, char close) { // span of parenthesized string starting at *str
  int depth = 0; char *s, open = *str;
  for (s = str; *s; ++s)
    if      (*s == close && --depth == 0) {++s; break;}
    else if (*s == open)    ++depth;
  return s - str;
}

char *json_value (char *json, char *_key) { // { "key1": "val1", "key2": 2 } cat
  if (!json || !_key) return NULL;
  if (*_key == '"') assert ("don't pass quotes to json_value");
  char x[999], *key = fmt(x,"\"%s\"",_key);
  char *val = strstr (json, key);
  if (!val) return NULL;
  val += strlen(key);       //if (*val == '"') ++val; // " after key
  val += strspn (val," :"); // skip :
  if (*val == '"') return extract_between (val, "\"", "\""); // string
  if (*val == '{') return parenthesized (val, '{', '}'); // object
  if (*val == '[') return parenthesized (val, '[', ']'); // list
  return strndup(val,strcspn(val,",}]"));
}

float json_numval (char *json, char *_key) {
  char *pos = "yes,true,positive,present"; // checked, ongoing
  char *neg = "no,false,negative,absent"; // unknown, not present
  char *val = json_value (json, _key);
  float num = (!val || !*val) ? 0 : strstr(neg,val) ? 0 : strstr(pos,val) ? 1 : atof(val);
  free (val); 
  return num;
}

char *next_token (char **text, char *ws) {
  if (!ws) ws = default_ws; 
  char *beg = *text + strspn (*text, ws); // skip whitespace
  if (!*beg) return NULL;
  char *end = beg + strcspn (beg, ws); // end of non-white
  *text = (*end) ? (end+1) : end;
  *end = 0;
  return beg;
}

void lowercase_stemmer (char *word, char *stem) {
  char *s;
  strncpy (stem, word, 999);
  for (s = stem; *s; ++s) *s = tolower ((int) *s);
}

void stem_word (char *word, char *stem, char *type) {
  switch (*type) {
  case 'K': kstem_stemmer (word, stem); break;
  case 'L': lowercase_stemmer (word,stem); break;
  default: strncpy(stem,word,999);
  }
  //if (strcmp(word,stem)) printf ("%s: %s -> %s\n", type, word, stem);
}

hash_t *load_stoplist () {
  hash_t *H = open_hash (NULL, "w");
  char line[1000], path[1000], *dir = getenv ("STEM_DIR");
  assert (dir && "$STEM_DIR environment variable must be set\n");
  FILE *in = safe_fopen (fmt(path,"%s/default.stp",dir), "r");
  while (fgets (line, 1000, in)) {
    if (*line == '!' || *line == '#') continue; // skip comments
    csub(line," \t",0); // terminate at first space / tab
    key2id(H,line);
    //printf ("adding '%s' from %s to hash\n", line, path);
  }
  fclose(in);
  return H;
}

int stop_word (char *word) { // thread-unsafe: static
  static hash_t *stops = NULL;
  if (!stops) stops = load_stoplist();
  //if (has_key(stops,word)) printf ("%s is a stopword\n", word);
  return has_key (stops, word);
}

char **str2toks (char *str, char *ws, uint maxlen) {
  char *tok, **toks = new_vec (0, sizeof(char*));
  while ((tok = next_token (&str, ws))) {
    if (strlen(tok) > maxlen) continue;
    tok = strdup(tok);
    toks = append_vec (toks, &tok);
  }
  return toks;
}

void stem_toks (char **toks, char *type) {
  char stem[1000], **w, **end = toks+len(toks);
  for (w = toks; w < end; ++w) {
    stem_word (*w, stem, type);
    free (*w);
    *w = strdup(stem);
  }
}

void stop_toks (char **toks) { // thread-unsafe: static
  static hash_t *stops = NULL;
  if (!stops) stops = load_stoplist();
  char **v, **w, **end = toks+len(toks);
  for (v = w = toks; w < end; ++w) 
    if (has_key (stops,*w)) free(*w);
    else *v++ = *w;
  len(toks) = v - toks;
}

ix_t *toks2ids (char **toks, hash_t *ids) {
  char **w, **end = toks+len(toks);
  ix_t *vec = new_vec (0, sizeof(ix_t)), new = {0,1};
  for (w = toks; w < end; ++w) {
    new.i = !(*w && **w) ? 0 : ids ? key2id(ids,*w) : (uint) atoi(*w);
    if (new.i) vec = append_vec (vec, &new);
  }
  return vec;
}

char **toks2pairs (char **toks, char *prm) {
  char **pairs = new_vec (0, sizeof(char*)), buf[1000];
  uint ow = getprm(prm,"ow=",0), uw = getprm(prm,"uw=",0);
  uint n = len(toks), k = ow ? MIN(n,ow) : uw ? MIN(n,uw) : n;
  char *a, *b, **v, **w, **end = toks+n;
  for (v = toks; v < end; ++v) {
    char *single = strdup(*v);
    pairs = append_vec (pairs, &single);
    for (w = v+1; w < v+k && w < end; ++w) {
      if (ow || strcmp(*v,*w) <= 0) { a = *v; b = *w; } // keep order: v_w
      else                          { a = *w; b = *v; } // swap order: w_v
      char *pair = strdup (fmt(buf,"%s,%s",a,b));
      pairs = append_vec (pairs, &pair);
    }
  }
  return pairs;
}

void free_toks (char **toks) {
  if (!toks) return;
  char **t = toks-1, **end = toks+len(toks);
  while (++t < end) if (*t) free (*t);
  free_vec (toks);
}

int read_doc (FILE *in, char *buf, int n, char *beg, char *end) {
  char *p = buf, *eob = buf + n, doc = 0, stop = 0;
  while (fgets (p, eob-p, in)) {
    if (strstr (p, beg)) doc = 1;
    if (!doc) continue; // skip everything until beg
    if (strstr (p, end)) stop = 1;
    p += strlen(p);
    if (stop || p+1 >= eob) break;
  }
  *p = 0;
  if (doc && !stop) fprintf (stderr, "\nWARNING: truncating: %30.30s...%s\n", buf, p-10);
  //fprintf (stderr, "%d %80.80s\n", doc, buf);
  return doc;
}

char *get_xml_docid (char *str) {
  char *id = NULL;
  if (!id) id = extract_between (str, "<DOC id=\"", "\""); // LDC
  if (!id) id = extract_between (str, "<DOCNO>", "</DOCNO>"); // TREC: DOCNO is primary!
  if (!id) id = extract_between (str, "<DOCID>", "</DOCID>"); // TREC: DOCID secondary
  if (!id) {
    fprintf(stderr,"----------ERROR: cannot find docid in:\n%s\n----------\n",str);
    assert (id); }
  //assert (id && "cannot find <DOCID> in xml");
  //if (!*id) *id = extract_between (str, "<PMID>", "</PMID>"); // medline
  //if (!*id) *id = extract_between (str, "<id>", "</id>"); // wikipedia
  //if (!*id) *id = extract_between (str, " ucid=\"", "\""); // MAREC
  //if (!*id) *id = extract_between (str, "<xn:resourceID>", "</xn:resourceID>"); // AcquireMedia
  //erase_between (str, "<DOCID>", "</DOCID>", ' ');
  chop (id, " \t"); // chop whitespace around docid
  return id;
}
