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

uint spell_check1 (char *word, hash_t *known, float *df) {
  
  
  
  
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
  printf ("besr: %ld\n", t1-t0);
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

int main (int argc, char *A[]) {
  if (argc == 5) return do_spell_12 (A[1], atoi(A[2]), A[3], A[4]);
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
