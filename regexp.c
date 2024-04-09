// helper functions around the standard C regex
#include "regex.h"
#include "string.h"
#include "assert.h"
#include "types.h"
#include "vector.h"

// compile pattern into regex_t
regex_t re_compile(char *pattern, char *prm) {
  int flags = 0;
  if (strchr(prm,'B')) flags |= REG_NOSUB; // Boolean match / no match
  if (strchr(prm,'i')) flags |= REG_ICASE; // case-insensitive
  if (strchr(prm,'n')) flags |= REG_NEWLINE; // \n marks start/end of text
  if (strchr(prm,'x')) flags |= REG_EXTENDED; // use POSIX extensions
  regex_t RE;
  int err = regcomp(&RE, pattern, flags);
  if (err) {
    char buf[1000];
    regerror(err, &RE, buf, 999);
    fprintf(stderr, "ERROR: [%d] '%s' in '%s'\n", err, buf, pattern);
    assert(0 && "could not compile regex pattern");
  }
  return RE;
}

it_t *regmatch_to_it_and_free (regmatch_t *els) {
  uint i, nels = len(els);
  it_t *R = new_vec (nels, sizeof(it_t));
  for (i=0; i<nels; ++i)
    R[i] = (it_t) {els[i].rm_so, els[i].rm_eo};
  free_vec (els);
  return R;
}

// beg:end for all sub-elements of the 1st match of RE in text
it_t *re_find_els(regex_t RE, char *text) {
  int flags = 0;
  // REG_NOTBOL ... ^ does not match beginning of text
  // REG_NOTEOL ... $ does not match end of text
  // REG_STARTEND ... search text[a:z] vs text[:], pass a:z in match[0]
  int nels = RE.re_nsub + 1; // number of sub-expressions in RE
  regmatch_t *els = new_vec (nels, sizeof(regmatch_t));
  int err = regexec(&RE, text, nels, els, flags);
  if (err) len(els) = 0; // error => no elements matched
  return regmatch_to_it_and_free (els);
}

// beg:end for all matches of the entire RE in text
it_t *re_find_all(regex_t RE, char *text) {
  it_t *result = new_vec (0, sizeof(it_t)), new = {0,0};
  int flags = 0;
  regmatch_t match[1]; // no sub-expressions
  char *t = text;
  while (!regexec(&RE, t, 1, match, flags)) {
    new.i = match[0].rm_so + (t - text);
    new.t = match[0].rm_eo + (t - text);
    result = append_vec (result, &new);
    t = text + new.t; // skip over matched
  }
  return result;
}

// de-allocate regex_t
void re_free(regex_t RE) { regfree(&RE); }

#ifdef MAIN
#define arg(i) ((i < argc) ? argv[i] : NULL)
#define a(i) ((i < argc) ? argv[i] : "")

int do_scan(char *pattern, int all, char *prm) {
  regex_t RE = re_compile(pattern, prm);
  char *buf = NULL;
  size_t bufsz = 0;
  while (getline(&buf, &bufsz, stdin) > 0) {
    it_t *els = (all ? re_find_all(RE, buf)
		 :     re_find_els(RE, buf)), *e;
    for (e = els; e < els + len(els); ++e) {
      char *s = strndup (buf + e->i, (e->t - e->i));
      puts(s);
      free(s);
    }
    free_vec (els);
  }
  re_free(RE);
  return 0;
}

char *usage =
  "usage: re -all RE prm ... show all matches of RE in stdin\n"
  "          -sub RE prm ... show all sub-expressions of RE\n"
  "                  prm ... i:ignorecase, x:extended, n, B\n"
  ;

int main (int argc, char *argv[]) {
  if (argc < 2) return fprintf (stderr, "%s", usage);
  if (!strcmp(a(1),"-all")) return do_scan (a(2), 1, a(3));
  if (!strcmp(a(1),"-sub")) return do_scan (a(2), 0, a(3));
  return 0;
}

#endif
