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

FILE **fopen_files (uint n, char *pfx, char *mode) {
  uint i; char path[9999];
  mkdir_parent (pfx);
  char *_fmt = n>10000 ? "%s%05d" : n>1000 ? "%s%04d" : n>100 ? "%s%03d" : n>10 ? "%s%02d" : "%s%d";
  FILE **file = new_vec (n, sizeof(FILE*));
  for (i = 0; i < n; ++i)
    file[i] = safe_fopen (fmt (path,_fmt,pfx,i), mode);
  return file;
}

// close NUM files and remove if CLEAN not NULL
void fclose_files (FILE **file, char *pfx, char *clean) {
  uint i, n = len(file); char path[9999];
  char *_fmt = n>10000 ? "%s%05d" : n>1000 ? "%s%04d" : n>100 ? "%s%03d" : n>10 ? "%s%02d" : "%s%d";
  for (i = 0; i < n; ++i) if (file[i]) {
      fclose(file[i]);
      if (clean) remove (fmt (path,_fmt,pfx,i));
    }
  free_vec (file);
  if (clean) rmdir_parent (pfx);
}

// read lines from IN, sort into OUT[i] based on column j
void do_shard (FILE *in, FILE **out, char *prm) {
  int bin = -1;
  char *key = getprms (prm,"key=",NULL,",");
  uint col = getprm (prm,"col=",0);
  char *line = NULL; size_t sz = 0;
  while (getline (&line, &sz, in) > 0) {
    if (key || col) {
      char *val = key ? json_value (line,key) : tsv_value (line,col);
      uint code = murmur3 (val, strlen(val));
      bin = code % len(out);
      free(val);
    }
    else bin = (bin+1) % len(out);
    //fprintf (out[bin], "%u %u '%s' %s", bin, code, val, line);
    fputs (line, out[bin]);
  }
  if (line) free (line);
  if (key) free (key);
}

// merge lines: this is pointless: use "sort -m"
/*
void do_merge (FILE *out, FILE **IN, char *prm) {
  uint i, n = len(IN); // number of input files
  char **LINE = new_vec (n, sizeof(char*)); // next line for each file
  int *NB = new_vec (n, sizeof(size_t)); // length of ^^^
  size_t *SZ = new_vec (n, sizeof(size_t)); // allocated for ^^^
  for (i=0; i<n; ++i) NB[i] = getline (LINE+i, SZ+i, IN[i]);
  while (1) { // until all files exhausted
    // find next i: NB[i]>0 and LINE[i] <= LINE[j] for all j
    // linear scan, or heap, based on some column?
    fputs(LINE[i],out); // yield the line
    NB[i] = getline (LINE+i, SZ+i, IN[i]); // advance
  }
  free_vec (LINE); free_vec (SZ); free_vec (NB);
}
*/

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
  //"            merge   ... merge lines in each bin, write to stdout\n"
  //"            clean   ... remove bin files when done\n"
  ;

int main (int argc, char *argv[]) {
  if (argc < 2) return fprintf (stderr, "%s", usage);
  char *prm = argv[1];
  uint num = getprm(prm,"num=",100);
  char *pfx = getprms(prm,"pfx=","bin.",",");
  char *mode = strstr(prm,"append") ? "a" : "w";
  char *clean = strstr(prm,"clean");
  //char *merge = strstr(prm,"merge");
  FILE **files = fopen_files (num, pfx, mode);
  do_shard (stdin, files, prm);
  //if (merge) do_merge (files, key);
  fclose_files (files, pfx, clean);
  free (pfx);
  return 0;
}

