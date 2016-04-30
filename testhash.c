/*

   Copyright (C) 1997-2014 Victor Lavrenko

   All rights reserved. 

   THIS SOFTWARE IS PROVIDED BY VICTOR LAVRENKO AND OTHER CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
   COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
   OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "hash.h"

uint multiadd_hashcode (char *s) ;
uint murmur3 (const char *key, uint len) ;

extern ulong COLLISIONS;
extern off_t MMAP_MOVES;
//extern uint  HASH_FUNC; // 0:multiadd 1:murmur3 2:OneAtATime
//extern uint  HASH_PROB; // 0:linear 1:quadratic 2:secondary
//extern float HASH_LOAD; // 0.1 ... 0.9

int main (int argc, char *argv[]) {
  char *prm = argv [argc-1];
  uint wait = getprm(prm,"wait=",0);
  MAP_SIZE  = getprm(prm,"M=",256); MAP_SIZE = MAP_SIZE << 20;
  //HASH_FUNC = getprm(prm,"F=",0);
  //HASH_PROB = getprm(prm,"P=",0);
  //HASH_LOAD = getprm(prm,"L=",0.5);
  
  if (argc < 3) {
    fprintf (stderr, 
	     "usage: testhash -load HASH < ids\n"
	     "                -dump HASH > pairs\n"
	     "                -vrfy HASH < pairs\n"
	     "                -keys HASH > ids\n"
	     "                -drop HASH < ids > new\n"
	     "                -add  HASH < ids\n"
	     "                -k2i  HASH key\n"
	     "                -i2k  HASH id\n"
	     "                -dbg  HASH\n"
	     "                -rand 1-4 logN\n"
	     );
    return 1;
  }
  
  if (!strcmp(argv[1], "-keys")) {
    hash_t *h = open_hash (argv[2], "r");
    uint i, n = nkeys(h);
    for (i = 1; i <= n; ++i) printf ("%s\n", id2key(h,i));
    free_hash (h);      
    return 0;
  }
  
  if (!strcmp(argv[1], "-dump")) {
    hash_t *h = open_hash (argv[2], "r");
    uint i, n = nkeys(h);
    fprintf (stderr, "%s: %d keys\n", argv[2], n);
    for (i = 1; i <= n; ++i) printf ("%10d %s\n", i, id2key(h,i));
    free_hash (h);      
    return 0;
  }
  
  if (!strcmp(argv[1], "-load")) {
    time_t start = time(0); 
    vtime();
    hash_t *h = open_hash (argv[2], "w");
    ulong done = 0;
    char key[10000];
    while (fgets(key, 10000, stdin)) {
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
    char line[10000], key[10000];
    while (fgets(line, 10000, stdin)) {
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
    hash_t *h = open_hash (argv[2], "r");
    char key[10000];
    while (fgets(key, 10000, stdin)) {
      key [strlen(key)-1] = 0; // chop newline
      if (!has_key (h, key)) printf ("%s\n", key);
    }
    free_hash (h);
    return 0;
  }
  
  if (!strcmp(argv[1], "-add")) {
    hash_t *h = open_hash (argv[2], "a");
    char key[10000]; ulong done = 0;
    while (fgets(key, 10000, stdin)) {
      key [strlen(key)-1] = 0; // chop newline
      key2id (h,key);
      if (!(++done%1000)) show_progress(done/1000,0,"K keys added");
    }
    free_hash (h);
    return 0;
  }
  
  if (!strcmp(argv[1], "-k2i")) {
    hash_t *h = open_hash (argv[2], "r");
    char *key = argv[3];
    printf ("%10d %s\n", key2id (h,key), key);
    return 0;
  }

  if (!strcmp(argv[1], "-i2k")) {
    hash_t *h = open_hash (argv[2], "r");
    uint i = atoi(argv[3]);
    printf ("%10d %s\n", i, id2key (h,i));
    return 0;
  }
  
  
  if (!strcmp(argv[1], "-dbg")) {
    hash_t *h = open_hash (argv[2], "r");
    uint *I = h->indx, n = len(I), i;
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

