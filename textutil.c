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
#include "hl.h"
#include "matrix.h"
#include "timeutil.h"
#include "query.h"

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

// find 'c' in "s" and return pointer after 'c', or NULL
char *strchr1 (char *s, char c) { s = strchr(s,c); return s ? s+1 : 0; }

// find "t" in "s" and return pointer after "t", or NULL
char *strstr1 (char *s, char *t) { s = strstr(s,t); return s ? s+strlen(t) : 0; }

// pointer to last t in s or NULL
char *strrstr (char *s, char *t) {
  char *last = NULL;
  uint n = strlen(t);
  while ((s = strstr(s,t))) { last = s; s += n; }
  return last;
}

// return count of character 'c' in "s"
uint cntchr (char *s, char c) { uint n=0; while (*s) if (*s++ == c) ++n; return n; }

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

// in text replace a prefix of each "src" with "trg"
void psub (char *text, char *src, char *trg) {
  char *p = text; uint ns = strlen(src), nt = strlen(trg);
  if (nt > ns) assert (0 && "in ssub trg cannot be longer than src");
  while ((p = strstr(p,src))) {
    memcpy (p, trg, nt);
    p += ns;
  }
}


// wipe out HTML: &quot; &amp; &#x10f; -> ' '
void no_xml_refs (char *S) {
  char *s = S-1, *beg = 0, *r;
  while (*++s) {
    if (*s == '&') r = beg = s; // most recent start
    if (*s != ';') continue;
    // beg..s is a possible &quot;
    if (s - beg > 10) { beg = 0; continue; } // too long
    while (++r < s) // r..s must be [#a-zA-Z0-9]
      if (!(isalnum(*r) || *r == '#')) { beg = 0; continue; }
    memset(beg, ' ', s-beg+1);
    beg = 0;
  }
  spaces2space (S); // multiple -> single space
}

// in S find "&plusmn;" -> SRC -> TRG -> "+/-"
void gsub_xml_refs (char *S, hash_t *SRC, coll_t *TRG) {
  char *beg = S, *end = S;
  while ((beg = strchr(end,'&')) && // beg -> '&'
	 (end = strchr(beg,';'))) { // end -> ';'
    char tmp = end[1]; // save what was after ';'
    end[1] = '\0'; // null-terminate '&plusmn;'
    uint id = has_key(SRC,beg);
    if (id) { // we have a mapping for '&plusmn;'
      char *trg = get_chunk(TRG,id); // target '+/-'
      //printf("replacing '%s' -> '%s'\n", beg, trg);
      char *p = stpcpy (beg,trg);   // '&plusmn;' ->  '+/-'
      while (p <= end) *p++ = '\r'; // '+/-_____'
    }
    end[1] = tmp; // restore what was after ';'
  }
  squeeze (S, "\r", NULL);
}

void no_xml_tags0 (char *S) {
  char *s = S-1, *t = S, in = 0;
  while (*++s) {
    char escaped = (s > S) && (s[-1] == '\\'); // escaped char
    if      (*s == '<' && !escaped) { in = 1; }
    else if (*s == '>' && !escaped) { in = 0; }
    else if (!in) *t++ = *s;
  }
  *t = '\0';
}

int inside_tag (char t) {
  return (t==' ') || isalnum(t) || (ispunct(t) && (t != '>'));
}

void no_xml_tags (char *S) {
  csub(S,"\b",' '); // use Backspace (\b) as a special marker
  char *s = S, *t;
  while ((t = s = strchr(s,'<'))) { // maybe start of a tag
    if (s > S && s[-1] == '\\') {++s; continue; } // skip escaped \>
    while (inside_tag(*t)) ++t;
    if (*t == '>') // [s..t] is a <tag>
      while (s <= t) *s++ = '\b';
    else ++s;
  }
  squeeze (S, "\b", NULL);
}

// str will have no chars from drop[], only those in keep[]
void squeeze (char *str, char *drop, char *keep) {
  if (!str) return;
  if (!drop && !keep) drop = default_ws;
  char *s, *t;
  if (drop) { for (t=s=str; *s; ++s) if (!strchr (drop, *s)) *t++ = *s; }
  else      { for (t=s=str; *s; ++s) if  (strchr (keep, *s)) *t++ = *s; }
  *t = 0; // new string may be shorter
}

int alphanumeric (char *str) {
  char *s, *ok = "-";
  for (s=str; *s; ++s)
    if (!isalnum(*s) && !strchr(ok, *s)) return 0;
  return 1;
}

// squeeze out any non-alphanumeric characters
void alphanumerize (char *str) {
  char *s, *t, *ok = "-";
  for (t=s=str; *s; ++s)
    if (isalnum(*s) || strchr(ok, *s)) *t++ = *s;
  *t = 0;
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

// split, but with string separator
char **strsplit (char *str, char *sep) {
  uint n = strlen(sep);
  char **result = new_vec (0, sizeof(char*));
  char *beg = str, *end = NULL, *part = NULL;
  while ((end = strstr(beg,sep))) {
    part = strndup (beg, end-beg);
    result = append_vec (result, &part);
    beg = end + n;
  }
  part = strdup(beg);
  result = append_vec (result, &part);
  return result;
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

void stop_at_chr (char *text, char *chr) {
  uint n = strcspn(text,chr);
  text[n] = '\0';
}

void stop_at_str (char *text, char *str) {
  char *end = strstr(text, str);
  if (end) *end = '\0';
}

// 1 if S starts with T else 0
int begins(char *S, char *T) { return !strncmp(S,T,strlen(T)); }

// copy part of S before B
char *str_before (char *S, char *B) {
  char *p = strstr(S, B);
  return p ? strndup(S, p-S) : NULL;
}

// copy part of S after A
char *str_after (char *S, char *A) {
  char *p = strstr(S, A);
  return p ? strdup (p+strlen(A)) : NULL;
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

char *extract_between_nocase (char *buf, char *A, char *B){
  char *a, *b, *result = NULL;
  if (!(a = strcasestr (buf, A))) return 0; // A not found
  a += strlen (A);     // skip A itself
  if (!(b = strcasestr (a, B))) {
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

char *json_docid (char *json) {
  char *id = json_value (json, "docid");
  if (!id) id = json_value (json, "id");
  return id;
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

void json2text (char *json) { squeeze (json, "{}[]\"", NULL); }

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
    csub(line," \t\n\r",0); // terminate at first space / tab
    key2id(H,line);
    //fprintf(stderr, "'%s'\n", line);
    //printf ("adding '%s' from %s to hash\n", line, path);
  }
  fclose(in);
  return H;
}

int stop_word (char *word) { // thread-unsafe: static
  static hash_t *stops = NULL;
  if (!stops) stops = load_stoplist();
  //if (has_key(stops,word)) printf ("%s is a stopword\n", word);
  //fprintf(stderr, "%s -> %d\n", word, has_key (stops, word));
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

// drop tokens with length outside [lo,hi]
void keep_midsize_toks (char **toks, uint lo, uint hi) {
  char **v, **w, **end = toks+len(toks);
  for (v = w = toks; w < end; ++w) {
    uint sz = strlen(*w);
    if (sz < lo || sz > hi) free (*w);
    else *v++ = *w;
  }
  len(toks) = v -toks;
}

// drop tokens that don't start with a letter
void keep_wordlike_toks (char **toks) {
  char **v, **w, **end = toks+len(toks);
  for (v = w = toks; w < end; ++w) {
    alphanumerize(*w);
    if (isalpha(**w)) *v++ = *w;
  }
  len(toks) = v -toks;
}

// simpler replacement for parse_vec_txt in matrix.c
char **text_to_toks (char *text, char *prm) {
  char *stop = strstr(prm,"stop");
  char *stem = getprmp(prm,"stem=","L");
  char *ws = " \t\r\n~`!@#$%^&*()_-+=[]{}|\\:;\"'<>,.?/"; // default
  char **toks = str2toks (text, ws, 50);
  keep_wordlike_toks (toks);
  keep_midsize_toks (toks, 3, 15);
  if (stem) stem_toks (toks, stem);
  if (stop) stop_toks (toks);
  return toks;
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

void show_toks (char **toks, char *fmt) {
  if (!fmt) fmt = "%s\n";
  char **t = toks-1, **end = toks+len(toks);
  while (++t < end) printf(fmt, *t);
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

char *get_xml_title (char *xml) {
  char   *s = extract_between (xml, "<title>", "</title>");
  if (!s) s = extract_between (xml, "<head>", "</head>");
  if (s) no_xml_tags (s);
  chop (s, " ");
  return s;
}

char *get_xml_author (char *xml) {
  char   *s = extract_between (xml, "<author>", "</author>");
  if (!s) s = extract_between (xml, "<auth>", "</auth>");
  if (!s) s = extract_between (xml, "<src>", "</source>");
  if (s) no_xml_tags (s);
  chop (s, " ");
  return s;
}

// pointer after next opening <tag...>
char *get_xml_open (char *xml, char *tag) {
  char *p = xml+1; int n = strlen(tag);
  while ((p = strcasestr (p,tag))) { // look for "tag"
    if (p[-1]=='<' && (p[n]=='>' || p[n]==' ')) // found <tag> or <tag ... ?
      //return (p-1); // including tag
      return strchr1(p+n,'>'); // excluding tag
    else p += n;
  }
  return NULL;
}

// pointer to next closing </tag...>
char *get_xml_close (char *xml, char *tag) {
  char *p = xml+2; int n = strlen(tag);
  while ((p = strcasestr (p,tag))) { // look for "tag"
    if (p[-2]=='<' && p[-1]=='/' && p[n]=='>') // found "</tag>" ?
      //return p+n+1; // including tag
      return p-2; // excluding tag
    else p += n;
  }
  return NULL;
}

// return string between <tag> and </tag>
char *get_xml_intag (char *xml, char *tag) {
  if ((tag[0]=='i') && (tag[1]=='d') && !tag[2]) return get_xml_docid (xml);
  char *beg = xml ? get_xml_open (xml, tag) : NULL; // find "<tag"
  //if (beg) beg = strchr(beg,'>'); // find end of "<tag ... >"
  char *end = beg ? get_xml_close (beg, tag) : NULL; // find "</tag"
  return end ? strndup (beg, end-beg) : NULL;
}

//char *get_xml_tag_attr (char *xml, char *tag, char *attr) {
//  char *end = xml ? get_xml_open (xml, tag) : NULL; // find end of "<tag ...>"
//  char *beg = end ? strRchr (xml, end, '<') : NULL; // find start
//  //if (beg) beg = strchr(beg,'>'); // find end of "<tag ... >"
//  char *end = beg ? get_xml_close (beg, tag) : NULL; // find "</tag"
//  return end ? strndup (beg, end-beg) : NULL;
//}

// ["ref","id"] -> <ref>...<id>_____</id>...</ref>
char *get_xml_intags (char *xml, char **tags) {
  char *span = xml, **tag = tags-1, **end = tags+len(tags);
  while (span && (++tag < end)) { // while we have span and more tags to find
    char *next = get_xml_intag (span, *tag); // try to match tag in span
    if (span != xml) free(span); // ^^^ will strdup
    span = next;
  }
  return span; // either NULL or we found all tags
}

// "ref.id" -> <ref>...<id>_____</id>...</ref>
char *get_xml_inpath (char *xml, char *path) {
  if (!strchr(path,'.')) return get_xml_intag (xml,path); // single tag
  path = strdup(path); // make a copy: split will insert \0 into it
  char **tags = split(path,'.'); // split path into tags
  char *match = get_xml_intags (xml, tags);
  free(path); free_vec(tags);
  return match;
}

// return all strings between <tag> and </tag>
char *get_xml_all_intag (char *xml, char *tag, char sep) {
  char *buf=0; int sz=0;
  while (1) {
    char *beg = xml ? get_xml_open (xml, tag) : NULL; // find "<tag"
    char *end = beg ? get_xml_close (beg, tag) : NULL; // find "</tag"
    if (!end) break;
    if (sz) memcat (&buf, &sz, &sep, 1);
    memcat (&buf, &sz, beg, end-beg);
    xml = end + strlen(tag);
  }
  return buf;
}

// -------------------------- concatenate docs --------------------------

// {trg},{src} -> {trg, src}
void append_json (char **trg, size_t *sz, char *src) {
  char *trg_end = strrchr (*trg,'}'); // find closing }
  char *src_beg = strchr1 (src,'{'); // right after first {
  if (!trg_end || !src_beg) return;
  *trg_end++ = ','; // replace closing { with ,
  *trg_end = '\0';
  stracat (trg, sz, src_beg); // wasteful, but clear
}

// <TRG>,<SRC> -> <TRG SRC>
void append_sgml (char **trg, size_t *sz, char *src) {
  char *trg_end = strstr(*trg, "</DOC>"); // immediately before </DOC>
  char *src_beg = strstr(src, "<DOC");
  if (src_beg) src_beg = strchr1(src_beg,'>'); // after <DOC...>
  if (!trg_end || !src_beg) return;
  *trg_end = '\0';
  stracat (trg, sz, src_beg); // wasteful, but clear
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
// O (nWords * textLen) + O (nHits * log nHits)
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

// return start+end offsets for all unescaped <tags>
// {i,j,k} i,j: start,end offsets of <tag>, k:zero
ijk_t *hits_for_xml_tags (char *S) {
  ijk_t *hits = new_vec (0, sizeof(ijk_t)), hit = {0,0,0};
  char *s = S-1, in = 0;
  while (*++s) {
    char escaped = (s > S) && (s[-1] == '\\'); // escaped char
    if (*s == '<' && !escaped) {
      in = 1;
      hit.i = s-S;
    }
    if (*s == '>' && !escaped && in) {
      in = 0;
      hit.j = s-S+1;
      hits = append_vec (hits, &hit);
    }
  }
  return hits; // sorted by construction
}

// drop hits that don't start with prefix
ijk_t *hits_with_prefix (char *S, ijk_t *H, char *prefix) {
  uint n = strlen(prefix);
  ijk_t *h, *hits = new_vec(0, sizeof(ijk_t));
  for (h = H; h < H+len(H); ++h)
    if (!strncmp(S+h->i, prefix, n))
      hits = append_vec (hits, h);
  return hits;
}




// score for snippet that doesn't contain any hits
//double score_empty (uint nwords, float eps) { return nwords * log(eps); }

// delta that results from adding dw instances of w
//double score_delta (uint w, int dw, int *seen, uint SZ, float eps) {
//  return - log (seen[w]/SZ + eps) + log ((seen[w]+=dw)/SZ + eps);
//}

// O(nWords): score = SUM_w log P(w|snippet)
float score_snippet (uint *seen, float eps) {
  double score = 0;// z = sumi(seen);
  uint i, n = len(seen);
  for (i=0; i<n; ++i) score += log (seen[i] + eps);
  return (float) score;
}

// SZ-byte span with highest number of distinct hits
// O(nHits * nWords) ... could be sped up by score_delta
jix_t best_span (ijk_t *hits, uint nwords, uint SZ, float eps) {
  jix_t best = {0, 0, -Infinity};
  ijk_t *H = hits, *end = H+len(H), *L = H-1, *R = H, *h; uint i;
  uint *seen = new_vec (nwords, sizeof(int)); // #times word[i] seen in [a..z]
  while (++L < end) { // for every left (L) border
    if (L > H) --seen [(L-1)->k]; // unsee word to left of L
    // advance R until |R-L| >= SZ seeing words R->k along the way
    for (; R<end && R->j < L->i+SZ; ++R) ++seen[R->k];
    double score = score_snippet (seen, eps);
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

// like best_span() but hits contain <tags> which don't eat up SZ
jix_t best_span_xml (ijk_t *hits, uint nwords, uint SZ, float eps) {
  jix_t best = {0, 0, -Infinity};
  ijk_t *H = hits, *end = H+len(H), *L = H-1, *R = H-1, *h;
  uint i, xml = 0, tag = 999999;
  uint *seen = new_vec (nwords, sizeof(int)); // #times word[i] seen in [a..z]
  while (++L < end) { // for every left (L) border
    // advance R until |R-L| >= SZ seeing words R->k along the way
    while (++R < end) {
      if (R->k == tag) xml += (R->j - R->i); // xml correction
      if (R->j - L->i >= SZ + xml) break; // span becomes too long
      if (R->k != tag) ++seen[R->k]; // saw a query term
    }
    double score = score_snippet (seen, eps);
    if (score > best.x) best = (jix_t) {L->i, (R-1)->j, score};
    if (0) { // debug
      for (i=0; i<nwords; ++i) fprintf (stderr, "%d ", seen[i]);
      fprintf (stderr, "= %.4f [%d..%d]", best.x, best.j, best.i);
      for (h=L; h<R; ++h) fprintf (stderr, " %d:%d", h->k, h->i);
      fprintf (stderr, "\n");
    }
    // unsee L for next iteration of the loop
    if (L->k != tag) --seen [L->k]; // unsee a word
    else xml -= L->j - L->i; // unsee an xml span
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

// all hits: O (nWords * textLen) + O(nH * log nH)
// best span: O (nWords * nHits) +
// broaden: O (textLen)
// total: O (nWords * textLen)
char *snippet2 (char *text, char **words, int sz, float *score) {
  if (!words || !len(words)) return strndup(text,sz); // no words => first sz bytes
  ijk_t *hits = hits_for_all_words (text, words);
  if (0) { // debug
    ijk_t *H = hits, *end = H+MIN(10,len(H)), *h;
    fprintf (stderr, "\nword:"); for (h=H; h<end; ++h) fprintf (stderr, "\t%d", h->k);
    fprintf (stderr, "\nbeg:"); for (h=H; h<end; ++h) fprintf (stderr, "\t%d", h->i);
    fprintf (stderr, "\nend:"); for (h=H; h<end; ++h) fprintf (stderr, "\t%d", h->j);
    fprintf (stderr, "\n");
  }
  jix_t span = best_span (hits, len(words), sz, 0.001);
  if (score) *score = span.x;
  span = broaden_span(text, span, sz);
  char *snip = strndup (text+span.j, span.i - span.j);
  if (0) fprintf (stderr, "%s\n", snip);
  free_vec (hits);
  return snip;
}

// snippet2 with highlighting
char *hl_snippet (char *text, char **words, int sz, float *score) {
  char *colors[] = fg_COLORS;
  char *buf = 0; int bufsz = 0;
  if (!words || !len(words)) return strndup(text,sz); // no words => first sz bytes
  ijk_t *hits = hits_for_all_words (text, words), *h;
  jix_t span = best_span (hits, len(words), sz, 0.001);
  if (score) *score = span.x;
  span = broaden_span(text, span, sz);
  uint prev = span.j, stop = span.i;
  //fprintf (stderr, "BEST_SPAN: [%d:%d]..%ld\n", prev, stop, strlen(text));
  for (h = hits; h < hits+len(hits); ++h) {
    if (h->i < prev) continue; // hit starts ouside window
    if (h->j > stop) break; // hit ends outside window
    char *color = colors[h->k % 6];
    memcat (&buf, &bufsz, text + prev, h->i - prev); // text before hit
    memcat (&buf, &bufsz, color, strlen(color));
    memcat (&buf, &bufsz, text + h->i, h->j - h->i); // hit itself
    memcat (&buf, &bufsz, RESET, strlen(RESET));
    prev = h->j;
  }
  //memcat (&buf, &bufsz, text + prev, stop - prev); // after last hit
  free_vec (hits);
  return buf;
}

// snippet2 with tagging
char *curses_snippet (char *text, char **words, int sz, float *score) {
  char *buf = 0; int bufsz = 0;
  if (!words || !len(words)) return strndup(text,sz); // no words => first sz bytes
  ijk_t *hits = hits_for_all_words (text, words), *h;
  jix_t span = best_span (hits, len(words), sz, 0.001);
  if (score) *score = span.x;
  span = broaden_span(text, span, sz);
  uint prev = span.j, stop = span.i;
  //fprintf (stderr, "BEST_SPAN: [%d:%d]..%ld\n", prev, stop, strlen(text));
  for (h = hits; h < hits+len(hits); ++h) {
    char color[9]; sprintf (color, "\t%c", '1' + (h->k % 6));
    if (h->i < prev) continue; // hit starts ouside window
    if (h->j > stop) break; // hit ends outside window
    memcat (&buf, &bufsz, text + prev, h->i - prev); // text before hit
    memcat (&buf, &bufsz, color, strlen(color));
    memcat (&buf, &bufsz, text + h->i, h->j - h->i); // hit itself
    memcat (&buf, &bufsz, "\t0", strlen("\t0"));
    prev = h->j;
  }
  //memcat (&buf, &bufsz, text + prev, stop - prev); // after last hit
  free_vec (hits);
  return buf;
}

// snippet2 with html highlighting
char *html_snippet (char *text, char **words, int sz, float *score) {
  char *buf = 0; int bufsz = 0;
  if (!words || !len(words)) return strndup(text,sz); // no words => first sz bytes
  ijk_t *hits = hits_for_all_words (text, words), *h;
  jix_t span = best_span (hits, len(words), sz, 0.001);
  if (score) *score = span.x;
  span = broaden_span(text, span, sz);
  uint prev = span.j, stop = span.i;
  //fprintf (stderr, "BEST_SPAN: [%d:%d]..%ld\n", prev, stop, strlen(text));
  for (h = hits; h < hits+len(hits); ++h) {
    char beg[9]; sprintf (beg, "<c%d>", 1 + (h->k % 6)); 
    char end[9]; sprintf (end, "</c%d>", 1 + (h->k % 6));
    //char *beg = "<b>", *end = "</b>";
    if (h->i < prev) continue; // hit starts ouside window
    if (h->j > stop) break; // hit ends outside window
    memcat (&buf, &bufsz, text + prev, h->i - prev); // text before hit
    memcat (&buf, &bufsz, beg, strlen(beg));
    memcat (&buf, &bufsz, text + h->i, h->j - h->i); // hit itself
    memcat (&buf, &bufsz, end, strlen(end));
    prev = h->j;
  }
  if (h->j < stop && prev < stop)
    memcat (&buf, &bufsz, text + prev, stop - prev); // after last hit
  free_vec (hits);
  return buf;
}

// slide sz-window over text, count words
// numWords * sz * textLen much worse than snippet2
//char *snippet3 (char *text, char **words, int sz) {
//}

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

// -------------------- Suffix Array --------------------

/*
char *sa_normalize (char *S, char *ws) {
  char *s;
  for (s = S; *s; ++s) if (strchr (ws, *s)) *s = ' ';
  lowercase (buf);
}

// return a concatenation A|B
char *sa_concatenate (char *A, char *B, char *sep) {
  char *buf=NULL, *s; uint sz=0;
  zprintf (&buf, &sz, "%s%s%s", A, sep, B);
  return buf;
}

// return pointers to suffix starting positions (unterminated)
char **sa_get_suffixes (char *text, char *ws) {
  char *p = text, *end = text + strlen(text);
  char **S = new_vec (0, sizeof(char*)), *p = text;
  while (p < end) {
    p += strspn(p, ws); // skip whitespace
    if (p >= end) break;
    S = append_vec(S, &p);
    p += strcspn(p, ws); // skip token
  }
  return S;
}

// lexicographically sort a suffix array
char *sa_sort_suffixes (char **starts) { sort_vec (starts, cmp_str); }
*/

// -------------------- n-gram match --------------------

//
// idea: for n = 5,4,3,2,1
//           for each n-gram in QRY
//               if n-gram in DOC: spans += {beg,end,n}
//       merge adjacent spans
//       snippet = max-covered size
//       highlight spans within snippet

/*
// lowercase & replace all punctuation with space
char *gram_prep (char *S) {
  char *s;
  for (s = S; *s; ++s) if (!isalnum(*s)) *s = ' ';
  lowercase (S);
}

// all starting positions for any n-gram
char **gram_starts (char *text) {
  char *s=text, **S = new_vec (0, sizeof(char*));
  while (*s) {
    while (*s == ' ') ++s; // skip space before word
    if (*s) beg = append_vec(beg, &s);
    while (*s && *s != ' ') ++s; // skip word itself
    if (1)  end = append_vec(end, &s);
  }
  return S;
}

char **n_grams (char *text, uint n, char *ws) {
  char *G = new_vec (0, sizeof(char*));
  char *beg = text, *end = text, *stop = text+strlen(text);
  while (end < stop) {
  }
}
*/

/*
float span2x (uint offs, uint size) { return offs + 1./size; }
uint x2offs (float x) { return (uint) x; }
uint x2size (float x) { return (uint) (1. / (x - (uint)x)); }

ijk_t *sjk2ijk (sjk_t *T, hash_t *H) {
  uint v, n = len(T);
  ijk_t *V = new_vec (n, sizeof(ijk_t));
  for (v=0; v<n; ++v) {
    V[v].i = key2id (H, T[v].s);
    V[v].j = T[v].j;
    V[v].k = T[v].k;
  }
  return V;
}

ix_t *sjk2ix (sjk_t *T, hash_t *H) {
  uint v, n = len(T);
  ix_t *V = new_vec (n, sizeof(ix_t));
  for (v=0; v<n; ++v) {
    V[v].i = key2id (H, T[v].s);
    V[v].x = span2x (T[v].j, T[v].k - T[v].j);
  }
  return V;
}
*/

float pack_span (uint beg, uint end) { return beg + 1./(end-beg+1); }
it_t unpack_span (float x) {
  double beg = floor(x), size = round(1./(x-beg)), end = beg + size - 1;
  return (it_t) {(uint)beg, (uint)end};
}

void show_tokens (sjk_t *T, char *tag) {
  printf("%s%s%s [%d]", fg_RED, tag, RESET, len(T));
  sjk_t *t = T-1, *end = T+len(T);
  while (++t < end)
    printf (" %s%d%s:%s%d%s %s", fg_CYAN, t->j, RESET, fg_CYAN, t->k, RESET, t->s);
  printf("\n");
}

void show_ijk (ijk_t *T, char *tag, uint n) {
  printf("%s%s%s [%d]:", fg_RED, tag, RESET, n);
  ijk_t *t = T-1, *end = T+n;
  while (++t < end)
    printf (" %s%d%s:%s%d%s:%s%d%s", fg_CYAN, t->j, RESET, fg_CYAN, t->k, RESET, fg_RED, t->i, RESET);
  printf("\n");
}

void free_tokens (sjk_t *T) {
  sjk_t *t = T-1, *end = T+len(T);
  while (++t < end) if (t->s) free(t->s);
  free_vec (T);
}

sjk_t *go_tokens (char *text) {
  sjk_t *T = new_vec(0,sizeof(sjk_t)), tok;
  char *p = text, *End = text + strlen(text);
  while (p < End) {
    while (*p && !isalnum(*p)) ++p; // find number or letter
    tok.j = p - text; // start of possible token
    while (isalnum(*p) || *p == '-') ++p; // possible end
    if (*p == '-') --p;
    if (*p == 's') --p;
    tok.k = p - text; // end of possible token
    if (tok.k <= tok.j) continue;
    tok.s = strndup (text + tok.j, tok.k - tok.j);
    lowercase (tok.s);
    T = append_vec (T, &tok);
    assert (tok.j <= End-text);
    assert (tok.k <= End-text);
  }
  return T;
}

void stop_tokens (sjk_t *T) {
  sjk_t *t;
  for (t = T; t < T+len(T); ++t) {
    if (!stop_word(t->s)) continue;
    free(t->s);
    t->s = NULL; // don't squeeze out these elements
  }
}

int skip_token(sjk_t *t) {
  return (t->k - t->j < 3) || !(t->s) || stop_word(t->s);
}

sjk_t *go_unigrams (sjk_t *T) {
  sjk_t *G = new_vec(0,sizeof(sjk_t)), *t;
  for (t = T; t < T+len(T); ++t) {
    if (skip_token(t)) continue;
    sjk_t unigram = {strdup(t->s), t->j, t->k};
    G = append_vec (G, &unigram);
  }
  return G;
}

sjk_t *go_bigrams (sjk_t *T) {
  sjk_t *G = new_vec(0,sizeof(sjk_t)), *p, *q;
  for (p = T; p < T+len(T)-1; ++p) {
    q = p+1;
    if (skip_token(p) || skip_token(q)) continue;
    sjk_t bigram = {acat3 (p->s," ",q->s), p->j, q->k};
    G = append_vec (G, &bigram);
  }
  return G;
}

// form an n-gram T[a..z] inclusive
sjk_t make_ngram (sjk_t *T, int a, int z) {
  char *buf=0; int sz=0, i;
  for (i=a; i<=z; ++i)
    if (T[i].s) zprintf (&buf, &sz, "%s ", T[i].s);
  if (sz>0 && buf[sz-1] == ' ') buf[sz-1] = '\0';
  return (sjk_t) {buf, T[a].j, T[z].k};
}

sjk_t *go_ngrams (sjk_t *T, uint n) {
  sjk_t *G = new_vec(0,sizeof(sjk_t));
  int i, N = len(T), m = n-1;
  for (i = 0; i < N-m; ++i) {
    if (!T[i].s || !T[i+m].s) continue;
    //if (skip_token(T+i) || skip_token(T+i+m)) continue;
    sjk_t ngram = make_ngram (T, i, i+m);
    assert (ngram.k - ngram.j > 0);
    G = append_vec (G, &ngram);
  }
  return G;
}

hash_t *ngrams_dict (char *qry, int n) {
  //printf("%s%d-grams_dict%s: %s\n", fg_MAGENTA, n, RESET, qry);
  hash_t *H = open_hash_inmem();
  sjk_t *tokens = go_tokens (qry);
  //show_tokens (tokens, "qry-toks");
  do {
    sjk_t *T = go_ngrams (tokens, n), *t;
    //show_tokens (T, "qry-grams");
    for (t=T; t < T+len(T); ++t) key2id (H, t->s);
    free_tokens(T);
  } while (--n > 0);
  free_tokens(tokens);
  return H;
}

float fCE(float *Q, float *D, float eps) {
  uint i, nQ = len(Q), nD = len(D), n = MIN(nQ,nD);
  double SQ = sumf(Q), SD = sumf(D), CE = 0;
  for (i=0; i<n; ++i)
    CE += (Q[i]/SQ) * log (D[i]/SD + eps);
  return CE;
}

float fCE2(float *Q, float *D, float *BG) {
  uint i, nQ = len(Q), nD = len(D), n = MIN(nQ,nD);
  double SQ = sumf(Q), SD = sumf(D), CE = 0;
  for (i=0; i<n; ++i)
    CE += (Q[i]/SQ) * log (D[i]/SD + BG[i]);
  return CE;
}

float *ngrams_freq (char *text, int n, hash_t *H) {
  float *F = new_vec (nkeys(H)+1, sizeof(float));
  sjk_t *tokens = go_tokens (text);
  do {
    sjk_t *T = go_ngrams (tokens, n), *t;
    for (t=T; t < T+len(T); ++t) ++F [has_key(H, t->s)];
    free_tokens(T);
  } while (--n > 0);
  free_tokens(tokens);
  return F;
}

double fake_bg (char *gram, hash_t *W, stats_t *S) {
  double P = 1, *CF = S->cf, NP = S->nposts; char *w;
  if (!gram || !*gram) return 1;
  while ((w = next_token (&gram, " "))) {
    if (stop_word(w)) continue; // P *= 1/1
    uint id = has_key(W,w);
    if (!id || id >= len(CF)) continue;
    P *= (CF[id] / NP);
  }
  return P;
}

// fake idf: P(w1,w2,w2) ~ P(w1) * P(w2) * P(w3)
float *ngrams_bg (hash_t *H, hash_t *W, stats_t *S) {
  uint id, n = nkeys(H);
  float *BG = new_vec (n+1, sizeof(float));
  for (id=1; id<=n; ++id) {
    char *gram = id2key(H,id);
    BG[id] = fake_bg(gram, W, S);
  }
  return BG;
}

// for each n-gram in H: how often we see it in tokens
float ngram_score (sjk_t *tokens, hash_t *H, int n) {
  uint N = nkeys(H), *seen = new_vec (N+1, sizeof(uint));
  do {
    sjk_t *T = go_ngrams (tokens, n), *t;
    for (t=T; t < T+len(T); ++t) {
      uint id = has_key(H, t->s);
      if (id) ++seen[id];
    }
    free_tokens(T);
  } while (--n > 0);
  float score = score_snippet (seen, 0.001);
  free (seen);
  return score;
}

ijk_t *ngram_matches (sjk_t *tokens, hash_t *H, int n) {
  //show_tokens (tokens, "doc-toks");
  ijk_t *matches = new_vec (0, sizeof(ijk_t));
  do {
    sjk_t *T = go_ngrams (tokens, n), *t;
    //show_tokens (T, "doc-grams");
    for (t=T; t < T+len(T); ++t) {
      uint id = has_key(H, t->s);
      ijk_t match = {n, t->j, t->k};
      //assert (match.k - match.j > 0);
      if (id) matches = append_vec (matches, &match);
    }
    free_tokens(T);
  } while (--n > 0);
  //show_ijk (matches, "matches", len(matches));
  return matches;
}

int disjoint_matches (ijk_t a, ijk_t b) { // a starts after b ends
  return (a.j > b.k) || (a.k < b.j);  // or a ends before b starts
}

ijk_t *merge_matches (ijk_t *M) {
  ijk_t *result = new_vec (0, sizeof(ijk_t));
  if (!len(M)) return result;
  sort_vec (M, cmp_ijk_j); // ascending start-of-gram
  ijk_t *m, this = M[0];
  for (m = M+1; m < M+len(M); ++m) {
    if (disjoint_matches (this, *m)) {
      result = append_vec (result, &this); // flush
      this = *m;
    } else { // merge intervals
      this.i = MAX(this.i, m->i);
      this.j = MIN(this.j, m->j);
      this.k = MAX(this.k, m->k);
      assert (this.i < 10 && this.j < this.k);
    }
  }
  result = append_vec (result, &this); // last hit
  //show_ijk (result, "merged", len(result));
  return result;
}

uint total_ijk_len (ijk_t *M) {
  uint sz = 0; ijk_t *m=M-1, *end = M+len(M);
  while (++m < end) sz += (m->k - m->j + 1);
  return sz;
}

uint total_sjk_len (sjk_t *M) {
  uint sz = 0; sjk_t *m=M-1, *end = M+len(M);
  while (++m < end) sz += (m->k - m->j + 1);
  return sz;
}


// | ToLower
// | Fields ... split on Unicode whitespace
// | Trim   ... chop leading & trailing ".,:?!-()[]{}/|<>"
// | filter ... drop if any char is not [a-z] and not '-'
// | emit W[i] if not in stoplist
//   emit W[i-1]" "W[i] if neither is in stoplist

// find [L,R] of length sz with max hit coverage
ijk_t best_ngram_span (ijk_t *M, uint sz, uint textLen) {
  if (!len(M)) return (ijk_t) {0, 0, MIN(sz,textLen)};
  //return (ijk_t) {0, 0, 500};
  ijk_t *L=M, *R, *end = M+len(M);
  ijk_t span = {L->k - L->j, L->j, L->k}; // [L]
  ijk_t best = span;
  for (R=M+1; R < end; ++R) {
    span.i += R->k - R->j; // add coverage of [R] to span coverage
    span.k  = R->k; // span [L,R] ends where R ends
    while ((L < R) && (R->k - L->j > sz)) { // span is too big
      span.i -= L->k - L->j; // remove coverage of [L]
      ++L;
      span.j = L->j; // span [L,R] begins where L begins
    }
    if (span.i > best.i) best = span;
  }
  return best;
}

void assert_ijk (ijk_t *M, uint nM, uint maxK, uint maxI) {
  ijk_t *m;
  for (m = M; m < M+nM; ++m) {
    assert (m->j <= maxK);
    assert (m->k <= maxK);
    assert (m->j <= m->k);
    assert (m->i <= maxI);
  }
}

void assert_sjk (sjk_t *M, uint maxK) {
  sjk_t *m;
  for (m = M; m < M+len(M); ++m) {
    assert (m->k <= maxK);
    assert (m->j < m->k);
  }
}

char *ngram_snippet (char *text, hash_t *Q, int gramsz, int snipsz, float *score) {
  uint textLen = strlen(text);
  // all unigrams in text, alnum, in-order {s:strndup j:begOffs k:endOffs}
  sjk_t *tokens = go_tokens (text);                   assert_sjk (tokens, textLen);
  // form n-grams, look up in Q, return {i:N, j:begOffs, k:endOffs}
  ijk_t *matches = ngram_matches (tokens, Q, gramsz); assert_ijk (matches, len(matches), textLen, 9);
  // merge overlapping [j:k), keep highest i:N
  ijk_t *M = merge_matches (matches), *m;             assert_ijk (M, len(M), textLen, 9);
  // find [j:k) w max coverage = sum of matching |j:k|
  ijk_t best = best_ngram_span (M, snipsz, textLen);  assert_ijk (&best, 1, textLen, textLen);
  //show_ijk (&best, "span", 1);
  uint prev = best.j, stop = best.k;
  char *buf = 0; int bufsz=0;
  for (m = M; m < M+len(M); ++m) {
    if (m->j < prev) continue; // hit starts outside span
    if (m->k > stop) break;    // hit ends outside span
    char bTag[9]; sprintf (bTag, "<g%d>", m->i);
    char eTag[9]; sprintf (eTag, "</g%d>", m->i);
    memcat (&buf, &bufsz, text + prev, m->j - prev); // text before match
    memcat (&buf, &bufsz, bTag, 4);
    memcat (&buf, &bufsz, text + m->j, m->k - m->j); // match itself
    memcat (&buf, &bufsz, eTag, 5);
    prev = m->k;
  }
  if (score) *score = (1. * best.i) / (best.k - best.j);
  free_vec (M);
  free_vec (matches);
  free_tokens (tokens);
  return buf;
}

// take text as-is and highlight it using n-grams
char *ngram_highlight (char *text, hash_t *Q, int gramsz) {
  uint textLen = strlen(text);
  sjk_t *tokens = go_tokens (text);
  assert_sjk (tokens, textLen);
  ijk_t *matches = ngram_matches (tokens, Q, gramsz);
  assert_ijk (matches, len(matches), textLen, gramsz);
  ijk_t *M = merge_matches (matches), *m;
  assert_ijk (M, len(M), textLen, gramsz);
  uint prev = 0, stop = textLen;;
  char *buf = 0; int bufsz=0;
  for (m = M; m < M+len(M); ++m) {
    if (m->j < prev) continue; // hit starts outside span
    if (m->k > stop) break;    // hit ends outside span
    char bTag[9]; sprintf (bTag, "<g%d>", m->i);
    char eTag[9]; sprintf (eTag, "</g%d>", m->i);
    memcat (&buf, &bufsz, text + prev, m->j - prev); // text before match
    memcat (&buf, &bufsz, bTag, 4);
    memcat (&buf, &bufsz, text + m->j, m->k - m->j); // match itself
    memcat (&buf, &bufsz, eTag, 5);
    prev = m->k;
  }
  free_vec (M);
  free_vec (matches);
  free_tokens (tokens);
  return buf;
}

char *hybrid_snippet (char *text, char **words, hash_t *Q, int gramsz, int snipsz, float *score) {
  char *snip1 = snippet2 (text, words, snipsz, score);
  char *snip2 = ngram_highlight (snip1, Q, gramsz);
  free (snip1);
  return snip2;
}

// return claim or paragraph that best covers the query
char *best_paragraph (index_t *I, char *text, char *qry, hash_t *_Q, int n, float *score) {
  //fprintf(stderr,"best_paragraph: text[%ld] qry[%ld] hash[%d] n:%d CR:%d\n",
  //strlen(text), strlen(qry), nkeys(_Q), n, cntchr(text,'\r'));
  char **P = split(text,'\t'); // assume Tab-separated paragraphs
  //fprintf(stderr,"%d paras\n", len(P));
  ix_t best = {0, -Infinity};
  uint i, nP = len(P);
  float *Q = ngrams_freq(qry, n, _Q);
  //float *BG = ngrams_bg (_Q, I->WORD, I->STATS);
  for (i=0; i<nP; ++i) {
    if (!P[i] || strlen(P[i]) < 10) continue; // too short
    float *D = ngrams_freq(P[i], n, _Q);
    float x = fCE(Q, D, 0.0001);
    if (x > best.x) best = (ix_t) {i, x};
    free_vec (D);
  }
  *score = best.x;
  char *result = strdup(P[best.i]);
  free_vec (P);
  free_vec (Q);
  //free_vec(BG);
  (void) I;
  return result;
}
