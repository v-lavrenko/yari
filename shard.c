#include "hash.h"
#include "textutil.h"

FILE **fopen_files (uint num, char *pfx, char *mode) {
  uint i;
  char path[9999];
  FILE **file = new_vec (num, sizeof(FILE*));
  for (i = 0; i < num; ++i) 
    file[i] = safe_fopen (fmt (path,"%s%d",pfx,i+1), mode);
  return file;
}

void fclose_files (FILE **file) {
  uint i;
  for (i = 0; i < len(file); ++i) if (file[i]) fclose(file[i]);
  free_vec (file);
}

void do_shard (FILE *in, FILE **out, char *prm) {
  uint col = getprm (prm,"col=",0);
  char *key = getprms (prm,"key=","id",',');
  char *line = NULL;
  size_t sz = 0;
  while (getline (&line, &sz, in) > 0) {
    char *val = col ? field_value (line,'\t',col) : json_value (line,key);
    uint bin = multiadd_hashcode (val) % len(out);
    //uint bin = murmur3 (id, strlen(val)) % num;
    fputs (line, out[bin]);
    if (val) free (val);
  }
  if (line) free (line);
  if (key) free (key);
}

/*
void do_merge (FILE **ins, char *key) {
  char *json = NULL;
  size_t sz = 0;
  uint i, n = len(ins);
  for (i = 0; i < n; ++i) {
    hash_t *H = open_hash (0, 0); // inmem
    FILE *in = ins[i]; if (!in) continue;
    fseek (in, 0, 0);
    while (getline (&json, &sz, in) > 0) {
      char *_id = json_value (json, key);
      uint id = key2id();
      uint bin = multiadd_hashcode (id) % len(out);
    //uint bin = murmur3 (id, strlen(id)) % num;
    fputs (json, out[bin]);
  }
  if (json) free (json);
    
    
  }
}
*/

char *usage = 
  "shard [prm] < LINES ... read stdin line-by-line, write to many files\n"
  "       prm: col=1   ... bin on value in column 1 (tab-separated)\n"
  "            key=id  ... bin on JSON key \"id\"\n"
  "            num=100 ... produce up to 100 bins\n"
  "            pfx=bin ... output to bin.{1,2...100}\n"
  "            append  ... append output files instead of overwriting\n"
  "            merge   ... merge lines in each bin, write to stdout\n"
  "            clean   ... remove bin files when done\n"
  ;

int main (int argc, char *argv[]) {
  if (argc < 2) return fprintf (stderr, "%s", usage); 
  char *prm = argv[1];
  uint num = getprm(prm,"num=",100);
  char *pfx = getprms(prm,"pfx=","bin",',');
  char *mode = strstr(prm,"append") ? "a" : "w";
  //char *merge = strstr(prm,"merge");
  FILE **files = fopen_files (num, pfx, mode);
  do_shard (stdin, files, prm);
  //if (merge) do_merge (files, key);
  fclose_files (files);
  free (pfx); free (mode);
  return 0;
}

