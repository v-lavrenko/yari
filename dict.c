/*

  Copyright (c) 1997-2024 Victor Lavrenko (v.lavrenko@gmail.com)

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

#include "hash.h"
#include "textutil.h"

uint multiadd_hashcode (char *s) ;
uint murmur3 (const char *key, uint len) ;
uint *href (hash_t *h, char *key, uint code) ;

extern ulong COLLISIONS;
extern off_t MMAP_MOVES;
//extern uint  HASH_FUNC; // 0:multiadd 1:murmur3 2:OneAtATime
//extern uint  HASH_PROB; // 0:linear 1:quadratic 2:secondary
//extern float HASH_LOAD; // 0.1 ... 0.9

int dict_dedup() {
  char dir[1000], *line = NULL; size_t sz = 0;
  sprintf (dir, "./dedup.%d", getpid()); // temporary directory
  hash_t *H = open_hash (dir,"w");
  while (getline (&line, &sz, stdin) > 0) {
    if (has_key (H,line)) continue; // already seen this line
    fputs (line,stdout); // if not: print
    key2id (H,line); // add to seen
  }
  if (line) free (line);
  free_hash (H);
  rm_dir (dir);
  return 0;
}

int dict_uniq(char *prm) {
  char *addup = strstr(prm,"addup");
  unsigned long NN=0;
  //ix_t *C = new_vec(0,sizeof(ix_t)), *c;
  ulong *C = new_vec(0,sizeof(ulong));
  hash_t *H = open_hash (0,0);
  char *line = NULL;
  size_t sz = 0;
  while (getline (&line, &sz, stdin) > 0) {
    if (*line == '\n') continue;
    ulong freq = addup ? atol(line) : 1;
    char *word = addup ? strchr(line,'\t')+1 : line;
    if (word == NULL+1) continue; // addup without \t
    uint id = key2id(H,word);
    if (id > len(C)) C = resize_vec (C, id);
    C[id-1] += freq;
    //C[id-1].i = id;
    //C[id-1].x ++;
    ++NN;
  }
  ulong **P = (ulong **)pointers_to_vec (C), **p;
  sort_vec (P, cmp_Ulong_ptr);
  if (!strstr(prm,"pct")) {
    for (p=P; p<P+len(P); ++p)
      printf ("%lu\t%s", **p, id2key(H,(*p-C+1)));
  } else {
    double sum = 0;
    for (p=P; p<P+len(P); ++p) sum += **p;
    for (p=P; p<P+len(P); ++p)
      printf ("%9.6f\t%s", 100.0*(**p)/sum, id2key(H,(*p-C+1)));
  }
  if (line) free (line);
  free_hash (H);
  free_vec (C);
  free_vec (P);
  return 0;
}

char *usage =
  "usage: dict     -load DICT < ids\n"
  "                -dump DICT > pairs\n"
  "                -vrfy DICT < pairs\n"
  "                -keys DICT > ids\n"
  "                -drop DICT < ids > new\n"
  "                -keep DICT < ids > old\n"
  "                 -add DICT < ids\n"
  "               -merge DICT += DICT2\n"
  "               -batch DICT += DICT2\n"
  "           -diff,tail DICT - DICT2\n"
  "                 -k2i DICT key\n"
  "                 -i2k DICT id\n"
  "                 size DICT\n"
  "                 -dbg DICT\n"
  "               -inmap MAP < src_trg_pairs\n"
  "              -outmap MAP\n"
  "        -usemap,col=1 MAP < tab_separated_lines\n"
  "                -rand 1-4 logN\n"
  "               -dedup ... suppress duplicate lines\n"
  "                -uniq ... sort | uniq -c | sort -rn\n"
  "               -addup ... add up freq[Tab]word\n"
  "           -load-ints DICT VEC < str_int_pairs\n"
  "           -dump-ints DICT VEC\n";

#define arg(i) ((i < argc) ? argv[i] : NULL)
#define a(i) ((i < argc) ? argv[i] : "")

int main (int argc, char *argv[]) {
  char *prm = argv [argc-1];
  uint wait = getprm(prm,"wait=",0);
  MAP_SIZE  = getprm(prm,"M=",256); MAP_SIZE = MAP_SIZE << 20;
  //HASH_FUNC = getprm(prm,"F=",0);
  //HASH_PROB = getprm(prm,"P=",0);
  //HASH_LOAD = getprm(prm,"L=",0.5);

  if (argc > 1 && (!strncmp(a(1), "-dedup", 6))) return dict_dedup();
  if (argc > 1 && (!strncmp(a(1), "-uniq", 5) ||
		   !strncmp(a(1), "-addup", 6))) return dict_uniq(a(1));

  if (argc < 3) return fputs (usage, stderr);

  //if (!strcmp(argv[1], "lock")) { MAP_MODE |= MAP_LOCKED;   ++argv; --argc; }
  //if (!strcmp(argv[1], "prep")) { MAP_MODE |= MAP_POPULATE; ++argv; --argc; }

  if (!strcmp(argv[1], "-keys")) {
    hash_t *h = open_hash (argv[2], "r");
    uint i, n = nkeys(h);
    for (i = 1; i <= n; ++i) puts (id2key(h,i));
    free_hash (h);
    return 0;
  }

  if (!strcmp(argv[1], "-dump")) {
    hash_t *h = open_hash (argv[2], "r");
    uint i, n = nkeys(h);
    fprintf (stderr, "%s: %d keys\n", argv[2], n);
    for (i = 1; i <= n; ++i) printf ("%d\t%s\n", i, id2key(h,i));
    free_hash (h);
    return 0;
  }

  if (!strcmp(argv[1], "-load")) {
    time_t start = time(0);
    vtime();
    hash_t *h = open_hash (argv[2], "w");
    ulong done = 0;
    char key[100000];
    while (fgets(key, 100000, stdin)) {
      key [strlen(key)-1] = 0; // chop newline
      //uint dup = has_key (h, key);
      uint id  = key2id  (h, key); assert (id);
      //char *k2 = id2key  (h, id);  assert (!strcmp(key,k2));
      //uint id2 = has_key (h, key); assert (id == id2);
      if (!(++done%1000)) show_progress(done/1000,0,"K keys");
      if (wait && time(0) >= start + wait) break;
    }
    fprintf (stderr, "\nprm: %s keys: %d remaps: %ld COLLISIONS: %ld  TIME: %.1f\n",
	     prm, nkeys(h), (long int)MMAP_MOVES, (long int)COLLISIONS, vtime());
    //fprintf (stderr, "[%.1fs] %lu keys\n", vtime(), done);
    free_hash (h);
    return 0;
  }

  if (!strcmp(argv[1], "-vrfy")) {
    hash_t *h = open_hash (argv[2], "r");
    uint id; ulong done = 0;
    char line[100000], key[100000];
    while (fgets(line, 100000, stdin)) {
      uint nf = sscanf (line, "%d %s", &id, key); assert (nf == 2);
      uint id2 = has_key (h, key);
      //char *k2 = id2key (h, id);
      if (id2 != id) printf ("\n%s: %d <> %d\n", key, id, id2);
      if (!(++done%1000)) show_progress(done/1000,0,"K pairs");
    }
    free_hash (h);
    fprintf (stderr, "[%.1fs] %lu pairs\n", vtime(), done);
    return 0;
  }

  if (!strcmp(argv[1], "-drop")) {
    hash_t *h = coll_exists (argv[2]) ? open_hash (argv[2], "r!") : NULL;
    char key[100000], *eol;
    while (fgets(key, 100000, stdin)) {
      if ((eol = strchr(key,'\n'))) *eol = '\0'; // chop newline
      if (!h || !has_key (h, key)) puts(key);
    }
    free_hash (h);
    return 0;
  }

  if (!strcmp(argv[1], "-keep")) {
    hash_t *h = open_hash (argv[2], "r!");
    char key[100000];
    while (fgets(key, 100000, stdin)) {
      key [strlen(key)-1] = 0; // chop newline
      if (has_key (h, key)) printf ("%s\n", key);
    }
    free_hash (h);
    return 0;
  }


  if (!strcmp(argv[1], "-add")) {
    hash_t *h = open_hash (argv[2], "a!");
    char key[100000]; ulong done = 0;
    while (fgets(key, 100000, stdin)) {
      key [strlen(key)-1] = 0; // chop newline
      key2id (h,key);
      if (!(++done%1000)) show_progress(done/1000,0,"K keys added");
    }
    free_hash (h);
    return 0;
  }

  if (!strcmp(argv[1], "-merge") && !strcmp(argv[3],"+=")) {
    hash_t *A = open_hash (argv[2], "a!");
    hash_t *B = open_hash (argv[4], "r");
    uint i, nB = nkeys(B), nA = nkeys(A);
    fprintf (stderr, "merge: %s [%d] += %s [%d]\n", A->path, nA, B->path, nB);
    for (i = 1; i <= nB; ++i) { // for each key in the table
      key2id (A, id2key (B,i));
      if (0 == i%10) show_progress (i, nB, "keys merged");
    }
    fprintf (stderr, "done: %s [%d]\n", A->path, nkeys(A));
    free_hash (A); free_hash (B);
    return 0;
  }

  if (!strcmp(argv[1], "-batch") && !strcmp(argv[3],"+=")) {
    char **keys = hash_keys (argv[4]);
    hash_t *A = open_hash (argv[2], "a");
    uint i, nB = len(keys), nA = nkeys(A);
    fprintf (stderr, "batch merge: %s [%d] += %s [%d]\n", A->path, nA, argv[4], nB);
    uint *ids = keys2ids (A,keys);
    fprintf (stderr, "done: %s [%d]\n", A->path, nkeys(A));
    for (i = 0; i < nB; ++i) if (keys[i]) free(keys[i]);
    free_vec (keys); free_vec (ids);
    free_hash (A);
    return 0;
  }

  if (!strcmp(argv[1], "-diff") && !strcmp(argv[3],"-")) {
    char *tail = strstr (argv[1], "tail");
    hash_t *A = open_hash (argv[2], "r");
    hash_t *B = open_hash (argv[4], "r!");
    uint i, nB = nkeys(B), nA = nkeys(A), nD = 0;
    fprintf (stderr, "diff: %s [%d] - %s [%d]\n", A->path, nA, B->path, nB);
    for (i = nA; i >= 1; --i) { // for each key in A
      char *key = id2key(A,i);
      uint code = A->code[i];
      uint *inB = href(B,key,code);
      if (!*inB) { ++nD; puts(key); }
      else if (tail) break; // stop on 1st key in B
    }
    fprintf (stderr, "diff: %d in %s - %s\n", nD, A->path, B->path);
    free_hash (A); free_hash (B);
    return 0;
  }

  if (!strcmp(argv[1], "-k2i")) {
    hash_t *h = open_hash (argv[2], "r");
    if (argc>3) {
      char *key = argv[3];
      printf ("%10d %s\n", key2id (h,key), key);
    }
    return 0;
  }

  if (!strcmp(argv[1], "-i2k")) {
    hash_t *h = open_hash (argv[2], "r");
    if (argc>3) {
      uint i = atoi(argv[3]);
      printf ("%10d %s\n", i, id2key (h,i));
    }
    return 0;
  }

  if (!strcmp(argv[1], "size")) {
    hash_t *h = open_hash (argv[2], "r");
    printf ("%d\n", nkeys(h));
    free_hash (h);
    return 0;
  }

  if (!strcmp(argv[1], "-dbg")) {
    hash_t *h = open_hash (argv[2], "r");
    uint *I = h->indx, n = len(I), i;
    printf ("keys: %u\n", nvecs(h->keys));
    printf ("code: %u\n", len(h->code));
    printf ("indx: %u\n", len(h->indx));
    printf ("#%9s %10s %10s %10s %s\n", "id", "slot", "code%n", "code", "key");
    for (i = 0; i < n; ++i) {
      if (I[i]) {
	char *key = get_chunk (h->keys, I[i]); // get key details
	uint code = h->code[I[i]];
	printf ("%10u %10u %10u %10u %s\n", I[i], i, code % n, code, key);
	//assert (k->code % n == i); // false for collisions
      } else printf ("%10s %10u\n", "", i);
    }
    free_hash (h);
    return 0;
  }

  if (!strncmp(argv[1], "-inmap", 6)) {
    char x[1000], line[100000];
    hash_t *SRC = open_hash (fmt(x,"%s.src",argv[2]), "w");
    coll_t *TRG = open_coll (fmt(x,"%s.trg",argv[2]), "w");
    while (fgets(line, 100000, stdin)) { // expect tab-separated: "src trg"
      char *src = line, *trg = strchr(line,'\t'), *eol = strchr(line,'\n');
      if (eol) *eol = '\0'; // strip newline
      if (trg) *trg++ = '\0'; // null-terminate src + advance to trg
      if (!has_key (SRC,src)) {
	uint id = key2id  (SRC, src);
	uint sz = strlen(trg)+1; // include \0
	//printf ("'%s' -> %d -> '%s' [%d]\n", src, id, trg, sz);
	put_chunk (TRG, id, trg, sz);
      }
      else printf ("skipping duplicate key: '%s' -> '%s'\n", src, trg);
    }
    printf ("%s [%d] -> %s [%d]\n", SRC->path, nkeys(SRC), TRG->path, nvecs(TRG));
    free_hash(SRC);
    free_coll(TRG);
    return 0;
  }

  if (!strncmp(argv[1], "-usemap", 7)) {
    char x[1000], line[100000];
    uint col = getprm(argv[1],"col=",1);
    hash_t *SRC = open_hash (fmt(x,"%s.src",argv[2]), "r!");
    coll_t *TRG = open_coll (fmt(x,"%s.trg",argv[2]), "r");
    while (fgets(line, 100000, stdin)) { // expect tab-separated columns
      noeol(line);
      char **F = split(line,'\t');
      uint i, NF = len(F);
      for (i=0; i<NF; ++i) {
	if (i) putchar('\t');
	if (i+1 == col) {
	  uint id = has_key (SRC, F[i]);
	  char *trg = id ? get_chunk (TRG,id) : "ERROR";
	  fputs(trg, stdout);
	} else fputs(F[i], stdout);
      }
      putchar('\n');
      free_vec (F);
    }
    free_hash(SRC);
    free_coll(TRG);
    return 0;
  }

  if (!strncmp(argv[1], "-outmap", 7)) {
    char x[1000];
    hash_t *SRC = open_hash (fmt(x,"%s.src",argv[2]), "r!");
    coll_t *TRG = open_coll (fmt(x,"%s.trg",argv[2]), "r");
    uint i, n = nkeys(SRC);
    for (i=1; i<=n; ++i) {
      char *src = id2key(SRC,i);
      char *trg = get_chunk(TRG,i);
      printf ("%s\t%s\n", src, trg);
    }
    free_hash(SRC);
    free_coll(TRG);
    return 0;
  }

  if (!strcmp(argv[1], "-rand")) {
    uint x[1021], p = 1021; (void) x;
    char *test = "mtx load:rcv,truth.qryids1docids2p=wrr3<4truth5-6loadground truth\n";
    ulong i = 0, n = (1l << atoi(argv[3])), k = strlen(test);
    if      (argv[2][0] == '1') while (++i<n) x[i%p] = random();
    else if (argv[2][0] == '2') while (++i<n) x[i%p] = rand();
    else if (argv[2][0] == '3') while (++i<n) x[i%p] = multiadd_hashcode (test + i%k);
    else if (argv[2][0] == '4') while (++i<n) x[i%p] = murmur3 (test + i%k, k-i%k);
    printf(" %lu random numbers ", i);
    return 0;
  }

  if (!strcmp(argv[1], "-load-ints")) {
    hash_t *H = open_hash (argv[2], "r");
    uint *V = open_vec (argv[3], "w", sizeof(uint)), noval = 0, nokey = 0, done = 0;
    V = resize_vec(V, nkeys(H)+1);
    char *line = NULL;
    size_t sz = 0;
    while (getline (&line, &sz, stdin) > 0) {
      char *key = line, *num = strchr (line, '\t');
      if (!num) { // no 2nd field
	noeol(line);
	if (++noval < 5) fprintf(stderr, "SKIP: no value in line '%s'\n", line);
	continue;
      }
      *num++ = '\0';
      uint id = has_key(H, key);
      if (!id) { // key not in H
	if (++nokey < 5) fprintf(stderr, "SKIP: %s has no key for '%s'\n", argv[2], line);
	continue;
      }
      V[id] = atol(num);
      if (!(++done%1000)) show_progress(done/1000,0,"K lines");
    }
    printf("\ndone: %d, no key: %d, no int: %d -> %s[%d]\n",
	   done, nokey, noval, argv[3], len(V));
    free(line);
    free_vec(V);
    free_hash(H);
    return 0;
  }

  if (!strcmp(argv[1], "-dump-ints")) {
    hash_t *H = open_hash (argv[2], "r");
    uint *V = open_vec (argv[3], "r", sizeof(uint));
    uint nH = nkeys(H)+1, nV = len(V), n = MIN(nH,nV), i;
    if (nV != nH) fprintf(stderr, "WARN: %s[%d] <> [%d]%s\n", argv[2], nH, nV, argv[3]);
    for (i = 1; i < n; ++i) printf ("%s\t%u\n", id2key(H,i), V[i]);
    free_vec(V);
    free_hash(H);
    return 0;
  }

  fprintf (stderr, "ERROR: incorrect %s", usage);
  return 0;

  ulong lines = 0;
  char *path = argv[1], phrase[10000];
  FILE *in = safe_fopen (argv[2], "r");
  vtime();
  hash_t *h = open_hash (path, "w");
  while (fgets(phrase, 10000, in)) {
    uint id = key2id (h, phrase);
    assert (!strcmp (id2key(h,id), phrase));
    if (0 == ++lines%10000000)
      printf ("[%.2fs] %.0fM lines -> %d keys 2^%u collisions 2^%u remaps\n",
	      vtime(), lines/1E6, nkeys(h), ilog2(COLLISIONS), ilog2(MMAP_MOVES));
  }
  free_hash (h);

  /*
  lines = 0;
  rewind(in);
  h = open_hash (path, "r");
  while (fscanf (in,"%[^\n]\n", phrase) == 1) {
    uint id = has_key (h, phrase);
    //assert (target == id);
    show_progress (++lines, 1E6, "M lines");
    }
  printf ("\nchecked %s: %ld lines -> %d keys\n", path, lines, nkeys(h));
  free_hash (h);
  */

  return (0);
}

  /*
  dict = hnew (10, "tryhash");
  printf ("[%.2f] mmapped a new hash\n", vtime());
  while (fscanf (in, "%[^\n]\n", phrase) == 1) {
    unsigned id = keyIn (dict, phrase);
    assert (!strcmp (i2key (dict, id), phrase));
    ++lines;  }
  printf ("[%.2f] filled an mmapped hash (%d lines)\n", vtime(), lines);
  hfree (dict);
  printf ("[%.2f] freed an mmapped hash\n", vtime());

  rewind (in);
  dict = hmmap ("tryhash", read_only);
  printf ("[%.2f] mmapped existing hash\n", vtime());
  while (fscanf (in, "%[^\n]\n", phrase) == 1) {
    unsigned id = key2i (dict, phrase);
    assert ((id > 0) && !strcmp (i2key (dict, id), phrase));
    --lines;  }
  assert (lines == 0);
  printf ("[%.2f] verified an mmapped hash\n", vtime());
  hfree (dict);
  printf ("[%.2f] freed an mmapped hash\n", vtime());

  rewind (in);
  dict = hnew (10, 0);
  printf ("[%.2f] allocced a new hash\n", vtime());
  while (fscanf (in, "%[^\n]\n", phrase) == 1) {
    unsigned id = keyIn (dict, phrase);
    assert (!strcmp (i2key (dict, id), phrase));
    ++lines;  }
  printf ("[%.2f] filled an allocced hash (%d lines)\n", vtime(), lines);
  hwrite (dict, "tryhash");
  hfree (dict);
  printf ("[%.2f] freed an allocced hash\n", vtime());

  rewind (in);
  dict = hread ("tryhash");
  printf ("[%.2f] read existing hash\n", vtime());
  while (fscanf (in, "%[^\n]\n", phrase) == 1) {
    unsigned id = key2i (dict, phrase);
    assert ((id > 0) && !strcmp (i2key (dict, id), phrase));
    --lines;  }
  assert (lines == 0);
  printf ("[%.2f] verified an allocced hash\n", vtime());
  hfree (dict);
  printf ("[%.2f] freed an allocced hash\n", vtime());
  */

