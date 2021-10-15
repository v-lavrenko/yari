#include "vector.h"

typedef struct {
  uint mask; //
  uint *indx;
  uint *code;
  char **keys;
} hash_t;

uint multiadd (char *key, uint sz) ;
uint murmur3 (const char *key, uint sz) ;

hash_t *new_hash (uint bits) {
  hash_t *H = calloc (1, sizeof(hash_t));
  uint N = 1 << bits;
  H->mask = N-1;
  H->indx = new_vec (N, sizeof(uint));
  H->keys = new_vec (0, sizeof(char*));
  //H->code = new_vec (0, sizeof(uint));
  return H;
}

void free_hash (hash_t *H) { free_vec(H->indx); free_vec(H->keys); free_vec (H->code); free (H); }

uint *href (hash_t *H, uint code, char *key, uint sz) {
  char **keys = H->keys;
  uint mask = H->mask, *indx = H->indx, i = code & mask, id=0;
  while (1) {
    id = indx[i];
    if (!id || !memcmp (keys[id-1], key, sz)) return indx+i;
    i = (i+1) & mask;
  }
}

void rehash (hash_t *H) {
  uint id, n = len(H->keys), N = (H->mask+1) << 1;
  H->mask = N-1;
  H->indx = resize_vec (H->indx, N);
  memset (H->indx, 0, N * sizeof(uint));
  for (id = 1; id <= n; ++id) {
    char *key = H->keys[id-1];
    uint sz = strlen(key);
    uint code = multiadd (key, sz);
    //uint code = H->code[id-1];
    uint *i = href (H, code, key, sz);
    (void) i;
  }
  
  
  //uint n0 = len(H->indx), n1 = n0<<1, mask1 = n1-1;
  //uint *indx0 = H->indx, *indx1 = new_vec (n1, sizeof(uint));  
}

uint key2id (hash_t *H, char *key, uint sz) {
  uint code = multiadd (key, sz), *i = href(H, code, key, sz);
  if (!*i) {
    key = strdup(key);
    H->keys = append_vec (H->keys, &key);
    //H->code = append_vec (H->code, &code);
    *i = len(H->keys);
    if (len(H->keys) > EMAXID * H->mask) rehash(H);
  }
  return *i;
}

char *id2key (hash_t *H, uint id) {
  return (H && id && (id <= len(H->keys))) ? H->keys[id-1] : NULL;
}


#ifdef MAIN

int do_freq () {
  char *line = NULL;
  ssize_t sz = 0; size_t lmt = 0;
  uint bits = 20, id = 0;
  hash_t *H = new_hash(bits);
  uint *F = new_vec((1<<bits),sizeof(uint));
  while ((sz = getline (&line, &lmt, stdin)) > 0) {
    if (line[sz-1] == '\n') line[--sz] = '\0';
    id = key2id(H, line, sz);
    //printf ("%s -> %d\n", line, id);
    if (!(id && id < len(F))) {
      fprintf (stderr, "%s -> %d < %d\n", line, id, len(F));
      return 1;
    }
    ++F[id];
  }
  for (id = 1; id <= len(H->keys); ++id)
    printf ("%d %s\n", F[id], id2key (H,id));
  free_vec (F);
  free_hash (H);    
  return 0;
}

char *usage =
  "hash2 -freq < words\n"
  ;

#define arg(i) ((i < argc) ? argv[i] : NULL)
#define a(i) ((i < argc) ? argv[i] : "")

int main (int argc, char *argv[]) {
  if (argc < 2) return fprintf (stderr, "%s", usage);
  if (!strcmp (a(1),"-freq")) return do_freq();
  return 1;
}

#endif
