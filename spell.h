
// delete/transpose/insert/substitute 'a' into string at X[0..n) at position i
int do_edit (char *X, int n, int i, char op, char a) ;

// insert all edits of word into set
int all_edits (char *word, uint nedits, hash_t *edits) ;

// *results = all edits that match the known hash, with scores
int known_edits (char *word, uint nedits, hash_t *known, float *score, ix_t **result) ;

// *best = id of edit that has highest score
int best_edit (char *word, uint nedits, hash_t *known, float *score, uint *best) ;

// aminoacid -> {i:amino, j:acid, x: MIN(F[amino],F[acid])}
jix_t try_split (char *word, hash_t *known, float *score) ;

// amino acid ic -> amino acidic
uint try_fuse (char *w1, char *w2, char *w3, hash_t *known, float *score) ;
