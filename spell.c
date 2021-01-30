#include "hash.h"
#include "textutil.h"
#include "timeutil.h"

//////////////////////////////////////////////////////////////////////////////// Norvig spell

// delete/transpose/insert/substitute 'a' into string at X[0..n) at position i
int do_edit (char *X, int n, int i, char op, char a) {
  int ok = 0;
  if (op == '=' && i < n) {ok=1; X[i] = a; } // substitute 
  if (op == '-' && i < n)   {ok=1; while (++i <= n) X[i-1] = X[i]; } // delete 
  if (op == '^' && i < n-1) {ok=1; char tmp=X[i]; X[i]=X[i+1]; X[i+1]=tmp; } // transpose
  if (op == '+' && i < n+1) {ok=1; do X[n+1] = X[n]; while (--n >= i); X[i] = a; } // insert
  return ok;
}

// insert all edits of word into set
int all_edits (char *word, uint nedits, hash_t *edits) {
  int n = strlen(word), pos;
  char *edit = calloc(n+3, 1);
  char *op, *ops = nedits > 2 ? "-^" : "-^=+"; // no ins/sub if 3+ edits
  for (op = ops; *op; ++op) { // no substitution if deleting/transposing
    char *sub, *subs = (*op=='-' || *op=='^') ? "a" : "abcdefghijklmnopqrstuvwxyz";
    for (sub = subs; *sub; ++sub) {
      for (pos = 0; pos <= n; ++pos) {
	memcpy(edit,word,n+1);
	int ok = do_edit (edit,n,pos,*op,*sub);
	if (!ok) continue; // could not do an edit
	if (nedits > 1) all_edits (edit, nedits-1, edits); // more edits to be done
	else if (edits) key2id (edits,edit); // leaf => insert into hash
      }
    }
  }
  free(edit);
  return 0;
}

// map edits to known words, assign scores
ix_t *score_edits (hash_t *edits, hash_t *known, float *score) {
  uint i, n = nkeys(edits);
  assert(len(score) > nkeys(known));
  ix_t *result = new_vec(0,sizeof(ix_t)), new = {0,0};
  for (i = 1; i <= n; ++i) {
    char *edit = id2key (edits, i);
    new.i = has_key (known, edit);
    new.x = new.i ? score[new.i] : 0;
    if (new.i) result = append_vec(result, &new);
  }
  return result;
}

// *results = all edits that match the known hash, with scores
int known_edits (char *word, uint nedits, hash_t *known, float *score, ix_t **result) {
  int n = strlen(word), pos; ix_t new = {0,0};
  char *edit = calloc(n+3, 1);
  char *op, *ops = nedits > 2 ? "-^" : "-^=+"; // no ins/sub if 3+ edits
  for (op = ops; *op; ++op) { // no substitution if deleting/transposing
    char *sub, *subs = (*op=='-' || *op=='^') ? "a" : "abcdefghijklmnopqrstuvwxyz";
    for (sub = subs; *sub; ++sub) {
      for (pos = 0; pos <= n; ++pos) {
	memcpy(edit,word,n+1);
	int ok = do_edit (edit,n,pos,*op,*sub);
	if (!ok) continue; // could not do an edit
	if (nedits > 1) known_edits (edit, nedits-1, known, score, result); // more edits
	else {
	  new.i = has_key (known, edit); // is this edit a known word?
	  new.x = new.i ? score[new.i] : 0; // 
	  if (new.i) *result = append_vec (*result, &new);
	}
      }
    }
  }
  free(edit);
  return 0;
}

// *best = id of edit that has highest score
int best_edit (char *word, uint nedits, hash_t *known, float *score, uint *best) {
  int n = strlen(word), pos;
  char *edit = calloc(n+3, 1);
  char *op, *ops = nedits > 2 ? "-^" : "-^=+"; // no ins/sub if 3+ edits
  for (op = ops; *op; ++op) { // no substitution if deleting/transposing
    char *sub, *subs = (*op=='-' || *op=='^') ? "a" : "abcdefghijklmnopqrstuvwxyz";
    for (sub = subs; *sub; ++sub) {
      for (pos = 0; pos <= n; ++pos) {
	memcpy(edit,word,n+1);
	int ok = do_edit (edit,n,pos,*op,*sub);
	if (!ok) continue; // could not do an edit
	if (nedits > 1) best_edit (edit, nedits-1, known, score, best); // more edits
	else {
	  uint id = has_key (known, edit); // is this edit a known word?
	  if (id && score[id] > score[*best]) *best = id;
	}
      }
    }
  }
  free(edit);
  return 0;
}

// aminoacid -> {i:amino, j:acid, x: MIN(F[amino],F[acid])}
jix_t try_split (char *word, hash_t *known, float *score) {
  jix_t best = {0, 0, 0};
  int n = strlen(word), k;
  for (k = 1; k < n; ++k) {
    char *wi = strndup(word,k), *wj = strndup(word+k,n-k);
    uint i = has_key(known,wi), j = has_key(known,wj);
    float x = (i && j) ? MIN(score[i],score[j]) : 0;
    if (x > best.x) best = (jix_t) {j, i, x};
    free(wi); free(wj);
  }
  return best;
}

// amino acid ic -> amino acidic
uint try_fuse (char *w1, char *w2, char *w3, hash_t *known, float *score) {
  int n1 = strlen(w1), n2 = strlen(w2), n3 = strlen(w3);
  char *w12 = calloc(n1+n2+1,1); strcat(w12,w1); strcat(w12,w2);
  char *w23 = calloc(n2+n3+1,1); strcat(w23,w2); strcat(w23,w3);
  uint i12 = has_key(known,w12); float x12 = i12 ? score[i12] : 0;
  uint i23 = has_key(known,w23); float x23 = i23 ? score[i23] : 0;
  free(w12); free(w23);
  return x12 > x23 ? i12 : x23 > 0 ? i23 : 0;
}

uint spell_check (char *word, hash_t *known, float *F) {
  uint i0 = has_key (known, word), i1=0, i2=0;
  if (i0 && F[i0] > 5) return i0;
  best_edit (word, 1, known, F, &i1); 
  if (i1 && F[i1] > F[i0]) return i1;
  best_edit (word, 2, known, F, &i2);
  if (i2 && F[i2] > F[i0]) return i2;
  return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////

#ifdef MAIN

void qselect (ix_t *X, int k) ;
void print_vec_svm (ix_t *vec, hash_t *ids, char *vec_id, char *fmt);
void dedup_vec (ix_t *X);
float *vec2full (ix_t *vec, uint N, float def);
uint num_cols (coll_t *c) ;

int do_spell_1 (char *word, uint nedits, hash_t *known, float *cf) {
  ulong t0, t1, t2, t3, t4;
  hash_t *all = open_hash(0,0);
  t0=ustime(); all_edits (word, nedits, all);
  t1=ustime(); ix_t *scored = score_edits (all, known, cf);
  t2=ustime(); qselect (scored, 10);
  t3=ustime(); sort_vec (scored, cmp_ix_X);
  t4=ustime(); print_vec_svm (scored, known, word, "%.0f");
  printf ("edits: %ld known: %ld top: %ld sort: %ld\n", t1-t0, t2-t1, t3-t2, t4-t3);
  free_hash(all);
  free_vec(scored);
  return 0;
}

int do_spell_2 (char *word, uint nedits, hash_t *known, float *cf) {
  ulong t0, t1, t2, t3, t4, t5;
  ix_t *scored = new_vec (0, sizeof(ix_t));
  t0=ustime(); known_edits (word, nedits, known, cf, &scored);
  t1=ustime(); sort_vec (scored, cmp_ix_i);
  t2=ustime(); dedup_vec (scored);
  t3=ustime(); qselect (scored, 10);
  t4=ustime(); sort_vec (scored, cmp_ix_X);
  t5=ustime(); print_vec_svm (scored, known, word, "%.0f");
  printf ("known: %ld sort: %ld dedup: %ld top: %ld sort: %ld\n", t1-t0, t2-t1, t3-t2, t4-t3, t5-t4);
  free_vec(scored);
  return 0;
}

int do_spell_3 (char *word, uint nedits, hash_t *known, float *cf) {
  ulong t0, t1; uint best = 0;
  t0=ustime(); best_edit (word, nedits, known, cf, &best);
  t1=ustime(); char *corr = best ? id2key(known,best) : "";
  printf ("%s %s:%.0f\n", word, corr, cf[best]);
  printf ("best: %ld\n", t1-t0);
  return 0;
}

int do_spell_12 (char *word, uint n, char *_H, char *_M) {
  hash_t *H = open_hash (_H,"r");
  coll_t *M = open_coll (_M, "r+");
  ix_t *_row = get_vec_ro (M,1);
  float *F = vec2full (_row, num_cols(M), 0);
  free_coll(M);
  do_spell_3 (word, n, H, F); printf("----------\n");
  do_spell_2 (word, n, H, F); printf("----------\n");
  do_spell_1 (word, n, H, F); printf("----------\n");  
  free_vec(F);
  free_hash(H);
  return 0;
}

int do_fuse (char *w1, char *w2, char *w3, char *_H, char *_F) {
  hash_t *H = open_hash (_H,"r");
  coll_t *M = open_coll (_F, "r+");
  ix_t *_row = get_vec_ro (M,1);
  float *F = vec2full (_row, num_cols(M), 0);
  free_coll(M);
  uint id = try_fuse (w1, w2, w3, H, F);
  uint i1 = has_key(H,w1), i2 = has_key(H,w2), i3 = has_key(H,w3);
  char *word = id ? id2key(H,id) : "N/A";
  printf("%s:%.0f <- %s:%.0f %s:%.0f %s:%.0f\n",
	 word, F[id], w1, F[i1], w2, F[i2], w3, F[i3]);
  free_vec(F);
  free_hash(H);
  return 0;
}

int do_split (char *word, char *_H, char *_F) {
  hash_t *H = open_hash (_H,"r");
  coll_t *M = open_coll (_F, "r+");
  ix_t *_row = get_vec_ro (M,1);
  float *F = vec2full (_row, num_cols(M), 0);
  free_coll(M);
  jix_t s = try_split (word, H, F);
  char *w1 = s.i ? id2key(H,s.i) : "N/A";
  char *w2 = s.j ? id2key(H,s.j) : "N/A";
  uint id = has_key(H,word);
  printf("%s:%.0f -> %s:%.0f %s:%.0f\n", word, F[id], w1, F[s.i], w2, F[s.j]);
  free_vec(F);
  free_hash(H);
  return 0;
}

#define arg(i) ((i < argc) ? A[i] : NULL)
#define a(i) ((i < argc) ? A[i] : "")

int main (int argc, char *A[]) {
  if (argc < 2) {
    fprintf (stderr, 
	     "usage: spell -spell remdesivir 1 WORD WORD_CF\n"
	     "       spell -fuse  amino acid ic WORD WORD_CF\n"
	     "       spell -split coronavirus WORD WORD_CF\n"
	     );
    return 1;
  }
  if (!strcmp(a(1),"-fuse"))  return do_fuse (a(2), a(3), a(4), a(5), a(6));
  if (!strcmp(a(1),"-split")) return do_split (a(2), a(3), a(4));
  if (!strcmp(a(1),"-spell")) return do_spell_12 (a(2), atoi(a(3)), a(4), a(5));
  if (argc == 3) {
    hash_t *H = open_hash (0,0);
    double t0 = 1E3 * ftime();
    all_edits(A[1], atoi(A[2]), H);
    double t1 = 1E3 * ftime();
    uint n = nkeys(H);
    printf ("%.0fms: %d keys\n", t1-t0, n);
    //for (i = 1; i <= n; ++i) printf ("%d\t%s\n", i, id2key(H,i));  
    free_hash (H);
    return 0;
  }
  if (argc == 5) {
    do_edit (A[1], strlen(A[1]), atoi(A[2]), A[3][0], A[4][0]);
    printf ("'%s'\n", A[1]);
    return 0;
  }
  return 1;
}

#endif
