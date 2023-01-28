/*
  
  Copyright (c) 1997-2021 Victor Lavrenko (v.lavrenko@gmail.com)
  
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "vector.h"
#include "hash.h"
#include "textutil.h"

int *col_names (char **cols, int n, char *hdr) {
  hdr += strspn(hdr," #"); // skip initial
  hdr = strdup(hdr);
  char **F = split(hdr,'\t');
  int *nums = calloc(n,sizeof(int)), i, j, NF = len(F);
  for (i=0; i<n; ++i) {
    if (cols[i][0] == '\\') continue; // skip \literal
    for (j=0; j<NF; ++j)
      if (!strcmp(cols[i],F[j])) nums[i] = j+1;
    if (!nums[i]) {
      fprintf(stderr,"ERROR: no field '%s' in: '%s'\n", cols[i], hdr);
      exit(1);
    }
  }
  free_vec(F);
  free(hdr);
  return nums;
}

int *col_nums (char **cols, int n, char *hdr) {
  int i; char *s;
  for (i=0; i<n; ++i) 
    if (cols[i][0] != '\\') // skip \literal
      for (s=cols[i]; *s; ++s) 
	if (!isdigit(*s)) return col_names (cols, n, hdr);
  int *nums = calloc(n,sizeof(int));
  for (i=0; i<n; ++i) nums[i] = atoi(cols[i]);
  return nums;
}

void print_val(char *val, int eol) {
  if (val) {
    csub(val,"\t\r\n",' ');
    fputs(val,stdout);
  }
  putchar(eol ? '\n' : '\t');  
}

void cut_tsv (char *line, int *nums, int n, char **cols) {
  char **F = split(line,'\t');
  int i, NF = len(F);
  for (i = 0; i < n; ++i) {
    int literal = (cols[i][0] == '\\'), c = nums[i];
    char *val = literal ? (cols[i]+1) : (c > 0 && c <= NF) ? F[c-1] : "";
    print_val (val, (i+1==n));
  }
  free_vec(F);
}

void cut_json (char *line, char **cols, int n) {
  int i;
  for (i = 0; i < n; ++i) {
    char *val = json_value (line, cols[i]);
    print_val (val, (i+1==n));
    free (val);
  }
}

void cut_xml (char *line, char **cols, int n) {
  int i; char *val;
  for (i = 0; i < n; ++i) {
    if (cols[i][0] == ',') val = get_xml_all_intag (line, cols[i]+1, ','); // "@id"
    else                   val = get_xml_inpath (line, cols[i]); // "body.ref.id"
    print_val (val, (i+1==n));
    free (val);
  }
}

int json_like (char *line) { return line[strspn(line," ")] == '{'; }
int xml_like (char *line) { return line[strspn(line," \t")] == '<'; }

void cut_stdin (char **cols, int n) {
  size_t sz = 999999;
  int *nums = NULL, nb = 0;
  char *line = malloc(sz), type = 0;
  while (0 < (nb = getline(&line,&sz,stdin))) {
    if (line[nb-1] == '\n') line[nb-1] = '\0';
    if (!type) {
      type = json_like(line) ? 'J' : xml_like(line) ? 'X' : 'T';
      if (type == 'T') nums = col_nums (cols, n, line);
    }
    if (type == 'J') cut_json(line, cols, n);
    if (type == 'X') cut_xml (line, cols, n);
    if (type == 'T') cut_tsv (line, nums, n, cols);
  }
}

int strip_xml () {
  size_t sz = 999999;
  int nb = 0;
  char *line = malloc(sz);
  while (0 < (nb = getline(&line,&sz,stdin))) {
    csub (line, "\t\r", ' ') ; // tabs and CR -> space
    no_xml_tags(line); // erase <...>
    spaces2space (line); // multiple -> single space
    chop (line, " "); // remove leading / trailing space
    fputs (line, stdout);
  }
  return 0;
}

int map_refs (char *MAP) {
  size_t sz = 999999;
  int nb = 0;
  char *line = malloc(sz), x[999];
  hash_t *SRC = MAP ? open_hash (fmt(x,"%s.src",MAP), "r") : NULL;
  coll_t *TRG = MAP ? open_coll (fmt(x,"%s.trg",MAP), "r") : NULL;
  while (0 < (nb = getline(&line,&sz,stdin))) {
    if (MAP) gsub_xml_refs (line, SRC, TRG); // '&#xae;' -> '(R)'
    else no_xml_refs(line); // erase &#xae; 
    fputs (line, stdout);
  }
  free_hash(SRC);
  free_coll(TRG);
  return 0;
}

int wc_stdin () {
  size_t sz = 999999;
  unsigned long NB = 0, NW = 0, NL = 0;
  int nb = 0;
  char *line = malloc(sz);
  while (0 < (nb = getline(&line,&sz,stdin))) {
    NL += 1; // number of lines
    NB += nb; // number of bytes
    int ws = 1; char *s;    
    for (s = line; *s; ++s) {
      if (*s == ' ') ws = 1;
      else if (ws) { ws = 0; ++NW; } // space -> word transition
    }
    //erase_between (line, "<", ">", ' '); // remove all tags
    //fputs (line, stdout);
  }
  printf ("\t%ld\t%ld\t%ld\t lines/words/bytes\n", NL, NW, NB);
  return 0;
}

int nf_stdin () {
  size_t sz = 999999;
  int nb = 0;
  char *line = malloc(sz);
  while (0 < (nb = getline(&line,&sz,stdin))) {
    char **toks = split (line, ' ');
    printf("%d\t%d\n", len(toks), nb);
    free_vec(toks);
  }
  return 0;
}

void show_header (char **cols, int n) {
  int i; fputc ('#', stdout);
  for (i = 0; i < n; ++i) {
    char sep = (i < n-1) ? '\t' : '\n';
    fputs (cols[i], stdout); 
    fputc (sep, stdout);
  }
}

/*
void stdin_grams (char *prm) {
  uint k = getprm(prm,"gram=",2);
  size_t sz = 999999, nb = 0, i;
  char *line = malloc(sz);
  while (0 < (nb = getline(&line,&sz,stdin))) {
    if (line[nb-1] == '\n') line[nb-1] = '\0';
    for (i=0; i<nb-k; ++i) ...
  }  
}
*/

#define arg(i) ((i < argc) ? argv[i] : NULL)
#define a(i) ((i < argc) ? argv[i] : "")

char *usage = 
  "xcut 3 2 \\X 17    ... print columns 3, 2, 'X', 17 from stdin\n"
  "xcut age size type ... use 1st line to map age -> column number\n"
  "xcut age size type ... stdin = {JSON} records one-per-line\n"
  "xcut age size type ... stdin = <XML> documents one-per-line\n"
  "xcut -h age size   ... print header before cutting\n"
  "xcut -wc           ... faster than wc byte/word/line counter\n"
  "xcut -nf           ... awk '{print NF, length}'\n"
  "xcut -noxml        ... strip CR, <tags>, MAP: '&copy;' -> '(C)'\n"
  "xcut -refs [MAP]   ... MAP: '&amp;' -> '&' (see dict -inmap)\n"
  //  "xcut -lf 'trg'     ... add LF immediately after trigger 'trg'\n"
  ;

int main (int argc, char *argv[]) {
  if (argc < 2) return fprintf (stderr, "\n%s\n", usage);
  if (!strcmp(a(1),"-noxml")) return strip_xml();
  if (!strcmp(a(1),"-refs")) return map_refs(arg(2));
  if (!strcmp(a(1),"-wc")) return wc_stdin();
  if (!strcmp(a(1),"-nf")) return nf_stdin();
  //if (!strcmp(a(1),"-lf")) return LF_after(arg(2));
  if (!strcmp(a(1),"-h")) { ++argv; --argc; show_header(argv+1,argc-1); }
  cut_stdin(argv+1,argc-1);
  return 0;
}
