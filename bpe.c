#include "hash.h"
#include "textutil.h"

int load_tokens (char *out, char *hash) {
  ulong NR = 0;
  ushort *O = open_vec (out, "w", sizeof(ushort));
  hash_t *H = open_hash (hash, "r");
  char tok[1000];
  while (fgets (tok, 999, stdin)) {
    noeol(tok); // strip newline
    ushort id = key2id(H,tok);
    if (id) O = append_vec (O, &id);
    if (((++NR)>>20) & 1) show_progress ((NR>>20), 0, "M tokens"); 
  }
  fprintf(stderr, "\nDONE: %ld tokens -> %s\n", NR, out);
  free_vec(O);
  free_hash(H);
  return 0;
}

uint *seq_freqs (ushort *V, ushort nk) {
  uint *F = new_vec(nk+1, sizeof(uint));
  ushort *v, *vEnd = V+len(V);
  for (v = V; v < vEnd; ++v) ++F[*v]; // frequencies
  return F;
}

int mk_seq_index (char *_I, char *_V, char *_H) {
  ushort *V = open_vec (_V, "r", sizeof(ushort)), *v, *vEnd = V+len(V);
  hash_t *H = open_hash (_H, "r");
  uint id, nk = nkeys(H);
  fprintf(stderr, "[%.2f] indexing %s[%d] -> %s[%d]\n", vtime(), _V, len(V), _I, nk);
  uint *F = seq_freqs (V, nk);
  fprintf(stderr, "[%.2f] %d freqs: %d,%d..%d\n", vtime(), len(F), F[0], F[1], F[nk]);
  uint **P = new_vec(nk+1,sizeof(uint*));
  for (id = 1; id <= nk; ++id) P[id] = new_vec(F[id], sizeof(uint)); // pre-alloc
  for (id = 1; id <= nk; ++id) len(P[id]) = 0;
  fprintf(stderr, "[%.2f] pre-alloc: %d lists\n", vtime(), nk);
  for (v = V; v < vEnd; ++v) {
    uint id = *v, *slot = P[id] + len(P[id]);
    *slot = v - V + 1;
    ++len(P[id]);
  }
  fprintf(stderr, "[%.2f] filled: %d pos -> %d lists\n", vtime(), len(V), nk);
  for (id = 1; id <= nk; ++id)
    if (F[id] != len(P[id]))
      fprintf(stderr, "ERROR: %d freq %d != %d len\n", id, F[id], len(P[id]));
  fprintf(stderr, "[%.2f] checked: %d freq == length\n", vtime(), nk);
  coll_t *I = open_coll (_I, "w+");
  for (id = 1; id <= nk; ++id) put_vec (I, id, P[id]);
  free_coll(I);
  fprintf(stderr, "[%.2f] wrote: %d lists -> %s\n", vtime(), nk, _I);
  for (id = 1; id <= nk; ++id) free_vec (P[id]);
  free_vec(P);
  free_vec(V);
  free_hash(H);
  return 0;
}

typedef struct {
  coll_t *I;
  ushort *V;
  hash_t *H;
  float *P;
} seq_index_t;

void *open_seq_index(char *_I, char *_V, char *_H) {
  seq_index_t *s = calloc(1, sizeof(seq_index_t));
  if (_I) s->I = open_coll(_I, "r+");
  if (_V) s->V = open_vec(_V, "r", sizeof(ushort));
  if (_H) s->H = open_hash(_H, "r");
  s->P = new_vec (len(s->V)+1024, sizeof(float));
  return (void*)s;
}

void free_seq_index(void *p) {
  seq_index_t *s = (seq_index_t*) p;
  if (s->I) free_coll(s->I);
  if (s->V) free_vec(s->V);
  if (s->H) free_hash(s->H);
  if (s->P) free_vec(s->P);
  memset (p, 0, sizeof(seq_index_t));
  free(p);
}

// score positions P that match sequence Q in index I.
float *seq_find (float *P, coll_t *I, ushort *Q, int nq) {
  float *a=P, *b=P, *c=P, *pEnd = P + len(P);
  ushort *q;
  for (q = Q; q < Q+nq; ++q) {
    uint shift = q-Q; // shift wrt 1st query term
    uint *idx = get_vec_ro(I, *q), *i, *iEnd = idx+len(idx);
    if (len(idx) > 1000000) continue;
    for (i = idx; i < iEnd; ++i) {
      float *p = P + (*i) - shift;
      float *lo = MAX(P,p-5), *hi = MIN(pEnd,p+5), *mid = p;
      for (p = lo; p < hi; ++p) {
	//if (p < P) continue;
	*p += 1 / ABS(p-mid);;
	if      (*p > *a) {c=b; b=a; a=p;}
	else if (*p > *b) {c=b; b=p;}
	else if (*p > *c) {c=p;}
      }
    }
  }
  //for (c = P; c < pEnd; ++c) *c += 0.1;
  return a;
}

// return the most-likely completion of Q based on V & I (not malloc'ed)
ushort *seq_complete (void *p, ushort *Q, int nq, int *nc) {
  seq_index_t *s = (seq_index_t*) p;
  ushort *V = s->V; coll_t *I = s->I; float *P = s->P;
  //fprintf(stderr,"V,I,P\n");
  memset (P, 0, len(P) * sizeof(float));
  //fprintf(stderr,"memset\n");
  float *best = seq_find (P, I, Q, nq);
  //fprintf(stderr,"seq_find -> best pos %ld score %f\n", best-P, *best);
  uint beg = best-P+nq-1, end = beg;
  while (end < len(V) && V[end] > 3) ++end; // find ending </s>
  //fprintf(stderr,"beg..end\n");
  if (nc) *nc = end - beg; // number of completion tokens
  return V+beg;
}

// splits text on space and hashes tokens into ids.
ushort *text2ids (char *text, hash_t *H) {
  char *_text = strdup(text); // we will tokenize it
  char **toks = split(_text, ' '), **t;
  ushort *ids = new_vec(0, sizeof(ushort));
  for (t = toks; t < toks+len(toks); ++t) {
    ushort id = (ushort) has_key(H, *t);
    if (id) ids = append_vec(ids, &id);
  }
  free_vec(toks);
  free (_text);
  return ids;
}

char *ids2text (ushort *ids, int n, hash_t *H) {
  ushort *id;
  char *buf=0; int sz=0;
  for (id = ids; id < ids+n; ++id) {
    char *tok = id2key(H, *id);
    zprintf (&buf, &sz, "%s ", tok);
  }
  if (sz) buf[--sz] = '\0'; // chop trailing space
  return buf;
}

// text -> ints -> new_ints -> new_text
char *text_complete (void *index, char *text) {
  int nc=0;
  seq_index_t *s = (seq_index_t*) index;
  ushort *ids = text2ids (text, s->H);
  ushort *new_ids = seq_complete (index, ids, len(ids), &nc);
  char *new_text = ids2text (new_ids, nc, s->H);
  free_vec (ids);
  return new_text;
}

int test_index (char *_I, char *_V) {
  coll_t *I = open_coll (_I, "r+");
  ushort *V = open_vec (_V, "r", sizeof(ushort));
  float *P = new_vec (len(V)+10240, sizeof(float));
  fprintf(stderr, "[%.2f] loaded index %s[%d] sequence %s[%d]\n",
	  vtime(), _I, nvecs(I), _V, len(V));
  uint reps = 100, qlen = 1000;
  while (--reps > 0) {
    memset ((void*)P, 0, len(P) * sizeof(float));
    uint offs = random() % (len(V) - qlen - 1); // random offset in V
    float *p = seq_find(P, I, V+offs, qlen);
    fprintf(stderr, "[%.2f] %d -> %ld [%.1f] vs [%.1f %.1f %.1f]\n",
	    vtime(), offs, p-P, *p, P[offs], P[offs+1], P[offs+2]);
  }
  free_vec(P);
  free_vec(V);
  free_coll(I);
  return 0;
}

int test2_index (char *_I, char *_V, char *_H) {
  void *index = open_seq_index(_I, _V, _H);
  char *qry = "▁The ▁camera ▁captures ▁and ▁analyzes ▁images";
  char *comp = text_complete(index, qry);
  printf("%s | %s\n", qry, comp);
  free(comp);
  free_seq_index(index);
  return 0;
}

void sample (ushort *V, hash_t *H, long idx, uint sz, char how) {
  if (idx < 0) idx = random();
  idx %= (len(V) - sz); // wrap if V[idx:idx+sz] would overflow
  uint i;
  putchar('[');
  for (i=idx; i < idx+sz; ++i) {
    if (i > idx) fputs(", ", stdout); // comma-separate tokens
    if (how == 'i') { printf("%d", V[i]); continue; } // [1, 2, 3, 4, 5]
    char *tok = id2key(H,V[i]);
    if (how == 't') { printf ("'%s'", tok); continue; } // ['I', 'am', 'a']
  }
  puts("]"); // adds newline
}

int rnd_sample (char *vec, char *dic, char *prm) {
  ushort *V = open_vec (vec, "r", sizeof(ushort));
  hash_t *H = open_hash (dic, "r");
  long id = getprm(prm,"id=",0);
  uint sz = getprm(prm,"sz=",128);
  char *how = getprmp (prm, "how=", "tok");
  sample (V, H, id, sz, *how);
  free_vec(V);
  free_hash(H);
  return 0;
}

// ------------------------------ main ------------------------------
#ifdef MAIN

#define arg(i) ((i < argc) ? argv[i] : NULL)
#define a(i) ((i < argc) ? argv[i] : "")

int usage () {
  fprintf (stderr, 
	   "usage: bpe -load VEC DICT < tokens\n"
	   "           -rand VEC DICT [id=0,sz=128,how=tok]\n"
	   "           -indx IDX VEC DICT\n"
	   "           -test IDX VEC\n"
	   "           -test2 IDX VEC DICT\n"
	   );
  return 1;
}

int main (int argc, char *argv[]) {
  srandom(time(0));
  if (argc < 4) return usage();
  if (!strcmp(a(1),"-load")) return load_tokens (arg(2), arg(3));
  if (!strcmp(a(1),"-rand")) return  rnd_sample (arg(2), arg(3), a(4));
  if (!strcmp(a(1),"-indx")) return  mk_seq_index (arg(2), arg(3), arg(4));
  if (!strcmp(a(1),"-test")) return  test_index (arg(2), arg(3));
  if (!strcmp(a(1),"-test2")) return  test2_index (arg(2), arg(3), arg(4));
  return 0;
}

/* this is ugly...
#include <microhttpd.h>

static enum MHD_Result handler
(void *cls, // context argument from MHD_start_daemon
 struct MHD_Connection * connection,
 const char *url, // requested url
 const char *method, const char *version, // GET,POST and HTTP version
 const char *up_data, size_t * up_size, // from POST, PUT etc
 void **ptr // I can set this, will be oreserved across calls
 ) {
  const char * page = cls;
  int ret;

  if (strcmp(method, "GET")) return MHD_NO; // only support GET
  if (*upload_data_size) return MHD_NO; // upload data in a GET

  *ptr = NULL; // clear context pointer
  struct MHD_Response *response = MHD_create_response_from_buffer
    (strlen(page), (void*) page, MHD_RESPMEM_PERSISTENT);
  int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
  MHD_destroy_response(response);
  return ret;
}

#define PAGE "<html><head><title>libmicrohttpd demo</title>"	\
               "</head><body>libmicrohttpd demo</body></html>"

int start_httpd(char *prm) {
  uint port = getprm(prm,"port=",8000);
  uint flags = MHD_USE_THREAD_PER_CONNECTION, END = MHD_OPTION_END;
  struct MHD_Daemon *d = MHD_start_daemon
    (flags, // OR of MHD_FLAG
     port,
     NULL, NULL, // AcceptPolicyCallback (auth) + arg to it
     &handler, PAGE, // AccessHandlerCallback + arg to it
     END);
  assert (d && "MHD_start_daemon returned NULL");
  (void) getc (stdin); // wait for keypress
  MHD_stop_daemon(d);
  return 0;
}

*/

#endif
