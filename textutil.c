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
#define _GNU_SOURCE
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include "hash.h"
#include "textutil.h"

char *default_ws = " \t\r\n~`!@#$%^&*()_-+=[]{}|\\:;\"'<>,.?/";

char *endchr (char *s, char c, uint n) { // faster strrchr
  char *p = s + n - 1; // end of string
  while (p > s && *p != c) --p;
  return p > s ? p : NULL;
}

char *strRchr (char *beg, char *end, char key) {
  if (!key || !end || end < beg) return beg;
  while (end > beg && *end != key) --end;
  return end;
}

char *strchr1 (char *s, char c) { s = strchr(s,c); return s ? s+1 : 0; }

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

// in haystack replace every instance of "needle" with 'with'
void gsub (char *haystack, char *needle, char with) {
  char *p = haystack; uint n = strlen(needle);
  while ((p = strstr(p,needle))) {
    memset (p, with, n);
    p += n;
  }
}

// in str replace any occurence of chars from punctuation[] with nothing
void squeeze (char *str, char *what) {
  if (!str) return;
  if (!what) what = default_ws;
  char *s, *t; 
  for (t=s=str; *s; ++s) if (!strchr (what, *s)) *t++ = *s;
  *t = 0; // new string may be shorter
}

// replace: multiple paces -> single space
void spaces2space (char *str) {
  if (!str || !*str) return;
  char *s, *t;  
  for (t=s=str+1; *s; ++s)
    if (s[0] != ' ' || s[-1] != ' ') *t++ = *s;
  *t = 0; // new string may be shorter
}

void cgrams (char *str, uint lo, uint hi, uint step, char *buf, uint eob) {
  uint n, N = strlen(str);
  char *t, *s, *end = str + N - 1, *b = buf;
  while (str < end && *str == '_') ++str; // skip starting blanks
  while (end > str && *end == '_') --end; // skip ending blanks
  for (n = lo; n <= hi; n+=1) { // for each n-gram size
    for (s = str; s+n <= end+1; s+=step) { // starting position
      if (b+n+1 >= buf+eob) break; // make sure we fit buffer
      for (t = s; t < s+n; ++t) *b++ = *t;
      *b++ = ' ';
    }
  }
  *b = 0;
}

char *tsv_value (char *line, uint col) { // FIXME!!!
  uint f;
  for (f = 1; f < col && line; ++f) line = strchr1 (line, '\t');
  if (!line) return NULL;
  uint length = strcspn (line, "\t\r\n");
  return strndup(line,length);
}

char **split (char *str, char sep) {
  char **F = new_vec (0, sizeof(char*)), *s = str-1;
  if (*str) F = append_vec(F,&str);
  while (*++s) if (*s == sep) {
      *s++ = '\0'; // null-terminate previous field
      F = append_vec(F,&s); // next field
      --s;
    }
  return F;
}

uint split2 (char *str, char sep, char **_tok, uint ntoks) {
  char **tok = _tok, **last = _tok + ntoks - 1;
  *tok++ = str;
  for (; *str; ++str) if (*str == sep) { 
      *str++ = 0; 
      *tok++ = str; 
      if (tok > last) break;
    }
  return tok - _tok;
}

void noeol (char *str) {
  char *eol = strchr(str,'\n');
  if (eol) *eol = '\0';
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

uint atou (char *s) { // string to integer (unsigned, decimal)
  register uint U = 0, B = 10;
  register int zero = '0', nine = '9';
  for (; *s >= zero && *s <= nine; ++s) // only leading digits
    U = B*U + (*s - zero);
  return U;
}

void reverse (char *str, uint n) { 
  char *beg = str-1, *end = str + n, tmp;
  while (++beg < --end) { tmp = *beg; *beg = *end; *end = tmp; }
}

char *itoa (char*_a, uint i) {
  char *a = _a;
  while (i > 0) { *a++ = i%10 + '0'; i /= 10; }
  *a = 0;
  reverse (_a, a-_a);
  return a;
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

char closing_paren (char open) {
  return (open == '"' ? '"' :  open == '{' ? '}' : open == '[' ? ']' : 
	  open == '(' ? ')' :  open == '<' ? '>' : '\0'); 
}

uint parenspn (char *str) { // span of parenthesized string starting at *str
  char *s, open = *str, close = closing_paren (open);
  int depth = 0; 
  for (s = str; *s; ++s)
    if      (*s == open)    ++depth;
    else if (*s == close && --depth == 0) {++s; break;}
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

double json_numval (char *json, char *_key) {
  char *pos = "yes,true,positive,present"; // checked, ongoing
  char *neg = "no,false,negative,absent"; // unknown, not present
  char *val = json_value (json, _key);
  double num = (!val || !*val) ? 0 : strstr(neg,val) ? 0 : strstr(pos,val) ? 1 : atof(val);
  free (val); 
  return num;
}

char *json_pair (char *json, char *_str) {
  if (!json || !_str) return NULL;
  char *str = strcasestr(json,_str);
  if (!str) return NULL;
  char *beg = strRchr(json,str,'"'), *end = strchr(str,'"')+1; 
  if (beg == json || end == NULL+1) return NULL;
  char *sep = end + strcspn(end,":,]}");
  if (*sep == ':') { // "..str..": wantedValue
    end = sep+1 + strspn(sep+1," "); // skip spaces
    if (*end == '"') { end = strchr(end+1,'"'); assert(end); ++end; }
    else                     end += strcspn(end,",}]"); // numeric, keyword, ?
    //if (strchr("{[\"",*end)) end += parenspn(end); // {...} or [...] or "..."
  } else { // "wantedKey": "...str..."
    beg = strRchr(json,beg-1,':'); // key-value separator
    beg = strRchr(json,beg-1,'"'); // closing of key 
    beg = strRchr(json,beg-1,'"'); // opening of key
  }
  return strndup (beg, end-beg);
}

void json2text (char *json) { squeeze (json, "{}[]\""); }

void purge_escaped (char *txt) { // remove \n, \\n etc
  char *s = txt + strlen(txt);
  while (--s > txt) 
    if (s[-1] == '\\' || *s == '\\') *s = ' ';    
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

char *lowercase(char *_s) {
  char *s = _s; 
  if (s) for (; *s; ++s) *s = tolower((int) *s);
  return _s;
}

char *uppercase(char *_s) {
  char *s = _s; 
  if (s) for (; *s; ++s) *s = toupper((int) *s);
  return _s;
}

int cntchr (char *s, char c) { int n; for (n=0; *s; ++s) n += (*s == c); return n; }

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

char *toks2str (char **toks) { // ' '.join(toks)
  char *buf=0; int sz=0, i=-1, n = len(toks);
  while (++i<n) if (toks[i]) zprintf (&buf,&sz, "%s ", toks[i]);
  if (sz) buf[--sz] = '\0'; // chop trailing space
  return buf;  
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

char **vec2toks (ix_t *vec, hash_t *ids) {
  uint i, n = len(vec), sz = sizeof(char*);
  char **toks = new_vec (n, sz);
  for (i=0; i<n; ++i) toks[i] = id2str (ids, vec[i].i);
  return toks;  
}

ix_t *toks2vec (char **toks, hash_t *ids) {
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

char **readlines(char *path) {
  char *buf = NULL;
  size_t bufsz = 0;
  int sz = 0;
  FILE *in = safe_fopen(path,"r");
  char **lines = new_vec (0, sizeof(char*));
  while (0 < (sz = getline(&buf, &bufsz, in))) {
    if (buf[sz-1] == '\n') buf[--sz] = '\0'; // strip newline
    char *line = strdup(buf);
    lines = append_vec(lines,&line);
  }
  fclose(in);
  if (buf) free(buf);
  return lines;
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

// -------------------------- snippets --------------------------

// returns SZ chars around 1st match of QRY in TEXT
char *snippet1 (char *text, char *qry, int sz) {
  int pad = (sz - strlen(qry)) / 2; // padding before/after
  char *end = text + strlen(text);
  char *beg = strcasestr(text,qry); // find locus (start)
  if (!beg) return NULL;
  beg = MAX(text,beg-pad); // begin: padding before locus start
  end = MIN(end,beg+sz);   // end: SZ after begin
  beg = MAX(text,end-sz);
  return strndup (beg, end-beg);
}

// list of pointers to all (non-overlapping) occurrences of qry in text
char **strall (char *_text, char *qry) {
  char **result = new_vec (0, sizeof(char*)); 
  char *text = _text, *hit = 0; 
  uint qlen = strlen(qry);
  while ((hit = strstr(text,qry))) {
    result = append_vec (result, &hit);
    text = hit + qlen;
  }
  return result;
}

// result[i] = length of words[i] 
uint *word_lengths (char **words) {
  uint nw = len(words), i, *wlen = new_vec (nw, sizeof(uint)); 
  for (i=0; i<nw; ++i) wlen[i] = strlen(words[i]);
  return wlen;
}

// return spans of all occurrences of all words in text
// {i,j,k} i:start-offset j:end-offset k:word-identity
ijk_t *hits_for_all_words (char *_text, char **words) {
  ijk_t *hits = new_vec (0, sizeof(ijk_t)); 
  char **w, **wEnd = words + len(words);
  for (w = words; w < wEnd; ++w) {
    uint wlen = strlen(*w);
    char *text = _text, *hit;
    while ((hit = strcasestr(text,*w))) {
      ijk_t h = {hit-_text, hit-_text+wlen, w-words};
      hits = append_vec (hits, &h);
      text = hit + wlen;
    }
  }
  sort_vec (hits,cmp_ijk_i); // by increasing offset
  return hits;
}

// score for snippet that doesn't contain any hits
//double score_empty (uint nwords, float eps) { return nwords * log(eps); }

// delta that results from adding dw instances of w
//double score_delta (uint w, int dw, int *seen, uint SZ, float eps) {
//  return - log (seen[w]/SZ + eps) + log ((seen[w]+=dw)/SZ + eps);
//}

float score_snippet (int *seen, uint sz, float eps) {
  double score = 0, SZ = sz; uint i, n = len(seen);
  for (i=0; i<n; ++i) score += log (seen[i]/SZ + eps);
  return (float) score;
}

// SZ-byte span with highest number of distinct hits
jix_t best_span (ijk_t *hits, uint nwords, uint SZ, float eps) {
  jix_t best = {0, 0, -Infinity}; 
  ijk_t *H = hits, *end = H+len(H), *L = H-1, *R = H, *h; uint i;
  int *seen = new_vec (nwords, sizeof(int)); // #times word[i] seen in [a..z]
  while (++L < end) { // for every left (L) border
    if (L > H) --seen [(L-1)->k]; // unsee word to left of L
    // advance R until |R-L| >= SZ seeing words R->k along the way
    for (; R<end && R->j < L->i+SZ; ++R) ++seen[R->k];
    double score = score_snippet (seen, SZ, eps);
    if (score > best.x) best = (jix_t) {L->i, (R-1)->j, score};
    if (0) { // debug
      for (i=0; i<nwords; ++i) fprintf (stderr, "%d ", seen[i]);
      fprintf (stderr, "= %.4f [%d..%d]", best.x, best.j, best.i);
      for (h=L; h<R; ++h) fprintf (stderr, " %d:%d", h->k, h->i);
      fprintf (stderr, "\n");
    }
  }
  free_vec (seen);
  return best;
}

// find whitespace closest to position in text, no farther than cap
int nearest_ws (char *text, int position, int cap) {
  int L = position, R = position;
  while (!isspace(text[R]) && text[R]) ++R;
  while (!isspace (text[L]) && L > 0) --L;
  int dR = R-position, dL = position-L;
  if (dR > cap && dL > cap) return position;
  return (dR < dL) ? R : L;
}

jix_t broaden_span (char *text, jix_t span, int sz) {
  int L = span.j, R = span.i, N = strlen(text);
  int got = R-L, pad = (sz-got)/2;
  if (pad < sz/10) return span; // close enough
  if (N <= sz) return (jix_t) {0, N, span.x};
  R = MIN(N,R+pad);
  L = MAX(0,R-sz);  L = nearest_ws(text,L,20);
  R = MIN(N,L+sz);  R = nearest_ws(text,R-1,20);
  return (jix_t) {L, R, span.x};
}

char *snippet2 (char *text, char **words, int sz) {
  if (!words || !len(words)) return strndup(text,sz); // no words => first sz bytes
  ijk_t *hits = hits_for_all_words (text, words);
  if (0) { // debug
    ijk_t *H = hits, *end = H+MIN(10,len(H)), *h;
    fprintf (stderr, "\nword:"); for (h=H; h<end; ++h) fprintf (stderr, "\t%d", h->k);
    fprintf (stderr, "\nbeg:"); for (h=H; h<end; ++h) fprintf (stderr, "\t%d", h->i);
    fprintf (stderr, "\nend:"); for (h=H; h<end; ++h) fprintf (stderr, "\t%d", h->j);
    fprintf (stderr, "\n");
  }
  jix_t span = best_span (hits, len(words), sz, 0.1);
  span = broaden_span(text, span, sz);
  char *snip = strndup (text+span.j, span.i - span.j);
  if (0) fprintf (stderr, "%s\n", snip);
  free_vec (hits);
  return snip;
}

//shortest span containing at least one hit for every word
/*
it_t range_hits (ijk_t *hits, uint nwords) {
  it_t *last = hits+len(hits)-1, *a = hits, *z = last, *h, *A=a, *Z=z;
  int *seen = new_vec (nwords, sizeof(uint)), need = 0;
  for (h=a; h<=z; ++h) ++seen [h->k]; // #times word [h->k] seen in hits [a..z]
  while (a < z && seen[z->k] > 1) {--seen[z->k]; --z; } // --z while no words lost
  while (1) {
    while (z < last && seen[need] < 1) ++seen [(++z)->k]; // ++z until see needed word
    while (a < last && seen[a->k] > 1) --seen [(a++)->k]; // ++a while no words lost
    if (a >= last || z >= last) break;
    if (z->t - a->t + wlen[z->k] < 
	Z->t - A->t + wlen[Z->k]) { A=a; Z=z; }
    --seen [need = a++->k];
  }
  free_vec(seen);
  return (it_t) {A - hits, Z - hits};
}
*/

it_t *range2hits (it_t *hits, it_t range, uint *wlen) {
  int *seen = new_vec (len(wlen), sizeof(uint));
  it_t *subset = new_vec (0, sizeof(it_t));
  it_t *A = hits + range.i, *Z = hits + range.t, *h;
  for (h = A; h <= Z; ++h) {
    if (++seen[h->i] > 1) continue; // add each term only once
    it_t hit = {h->t, h->t + wlen[h->i]};
    subset = append_vec (subset, &hit);
  }
  return subset;
}

// expand each hit to sz chars 
// TODO: adjust padding for JSON, XML, etc
void widen_hits (it_t *hits, char *text, uint sz) {
  int N = strlen(text); it_t *h; 
  for (h = hits; h < hits + len(hits); ++h) {
    h->i = nearest_ws (text, MAX(0,((int)h->i)-(int)(sz/2)), 100);
    h->t = nearest_ws (text, MIN(N,((int)h->t)+(int)(sz/2)), 100);
  }
}

// merge overlapping hits 
void merge_hits (it_t *hits, uint epsilon) {
  it_t *L, *R, *end = hits + len(hits);
  for (L = R = hits; R < end; ++R) {
    if (L->t + epsilon > R->i) L->t = R->t; // L,R overlap => merge
    else *L++ = *R;
  }
  len(hits) = L - hits;
}

// [{i,x}]: gap between hits[i] and hits[i-1] is x
ix_t *gaps_between_hits (it_t *hits) {
  uint i, n = len(hits);
  ix_t *gaps = new_vec (n-1, sizeof(ix_t));
  for (i=1; i<n; ++i) {
    gaps[i-1].i = i;
    gaps[i-1].x = hits[i].i - hits[i-1].t;
  }
  sort_vec (gaps, cmp_ix_X);
  return gaps;
}


/*
void fit_hits (it_t *hits, char *text, int sz) {
  it_t *A = hits, *Z = hits+len(hits)-1, *h;
  if (Z->t - A->i < sz) { // no need to cut down
    A->t = Z->t; len(hits) = 1;
    return widen_hits (hits, text, sz - (A->t - A->i));
  }
  int 
  
}
*/

/*


it_t *hits2cuts (it_t *hits) {
  it_t *cuts = new_vec (0, sizeof(it_t)), *h;
  for (h = hits+1; h < hits+len(hits); ++h) {
    it_t cut = {(h-1)->t+1
  }
  for (h = hits + best.i; h <= hits + best.t; ++h) {
    it_t new = { h->t, h->t + wlen[h->i] };
    keep = append_vec (keep, &new);
  }
  it_t *skip = new_vec
}
*/
