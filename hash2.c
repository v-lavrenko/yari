#include "vector.h"

typedef struct {
  uint mask; //
  uint *indx; 
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
  return H;
}

void free_hash (hash_t *H) { free_vec(H->indx); free_vec(H->keys); free (H); }

char *id2key (hash_t *H, uint id) {
  return (H && id && (id <= len(H->keys))) ? H->keys[id-1] : NULL;
}

uint key2id (hash_t *H, char *key, uint sz) {
  char **keys = H->keys;
  uint mask = H->mask, *indx = H->indx, code = multiadd (key, sz), i = (code & mask), id=0;
  //printf ("key:%s -> code:%d & %d -> i:%d\n", key, code, mask, i);
  while (indx[i]) {
    id = indx[i];
    //printf ("code:%d & %d -> i:%d -> id:%d -> %s\n", code, mask, i, id, keys[id-1]);
    if (!memcmp (keys[id-1], key, sz)) {
      //printf("match!\n");
      return id;
    }
    i = (i+1) & mask;
  } // indx[i] is empty -> no match
  key = strdup(key);
  H->keys = append_vec(H->keys, &key);
  id = len(H->keys);
  indx[i] = id;
  //printf ("added\n");
  return id;
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
