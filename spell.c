#include "hash.h"
#include "textutil.h"
#include "timeutil.h"

// delete/transpose/insert/substitute 'a' into string at X[0..n) at position i
int do_edit (char *X, int n, int i, char op, char a) {
  int ok = 0;
  if (op == '=' && i < n) {ok=1; X[i] = a; } // substitute 
  if (op == '-' && i < n)   {ok=1; while (++i <= n) X[i-1] = X[i]; } // delete 
  if (op == '^' && i < n-1) {ok=1; char tmp=X[i]; X[i]=X[i+1]; X[i+1]=tmp; } // transpose
  if (op == '+' && i < n+1) {ok=1; do X[n+1] = X[n]; while (--n >= i); X[i] = a; } // insert
  return ok;
}

int all_edits (char *X, uint nedits, hash_t *H) {
  int n = strlen(X), pos;
  char *Y = calloc(n+3, 1);
  char *op, *ops = nedits > 2 ? "-^" : "-^=+"; // no ins/sub if 3+ edits
  for (op = ops; *op; ++op) { // no substitution if deleting/transposing
    char *sub, *subs = (*op=='-' || *op=='^') ? "a" : "abcdefghijklmnopqrstuvwxyz";
    for (sub = subs; *sub; ++sub) {
      for (pos = 0; pos <= n; ++pos) {
	memcpy(Y,X,n+1);
	int ok = do_edit (Y,n,pos,*op,*sub);
	if (!ok) continue; // could not do an edit
	if (nedits > 1) all_edits (Y, nedits-1, H); // more edits to be done
	else if (H) key2id (H,Y); // leaf => insert into hash
      }
    }
  }
  free(Y);
  return 0;
}

int main (int argc, char *A[]) {
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
