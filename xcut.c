/*
  
  Copyright (c) 1997-2016 Victor Lavrenko (v.lavrenko@gmail.com)
  
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
    for (s=cols[i]; *s; ++s)
      if (!isdigit(*s)) return col_names (cols, n, hdr);
  int *nums = calloc(n,sizeof(int));
  for (i=0; i<n; ++i) nums[i] = atoi(cols[i]);
  return nums;
}

void cut_tsv (char *line, int *cols, int n) {
  char **F = split(line,'\t');
  int i, NF = len(F);
  for (i = 0; i < n; ++i) {
    int c = cols[i];
    char *val = (c > 0 && c <= NF) ? F[c-1] : "";
    char sep = (i < n-1) ? '\t' : '\n';
    fputs (val,stdout);
    fputc (sep,stdout);
  }
  free_vec(F);
}

void cut_json (char *line, char **cols, int n) {
  int i;
  for (i = 0; i < n; ++i) {
    char *val = json_value (line, cols[i]);
    char sep = (i < n-1) ? '\t' : '\n';
    if (val) { fputs(val,stdout); free (val); }
    fputc (sep, stdout);
  }
}
 
int json_like (char *line) { return line[strspn(line," ")] == '{'; }
 
void cut_stdin (char **cols, int n) {
  size_t sz = 999999;
  int *nums = NULL, nb = 0;
  char *line = malloc(sz), type = 0;
  while (0 < (nb = getline(&line,&sz,stdin))) {
    if (line[nb-1] == '\n') line[nb-1] = '\0';
    if (!type) {
      type = json_like (line) ? 'J' : 'T';
      if (type == 'T') nums = col_nums (cols, n, line);
    }
    if (type == 'J') cut_json(line, cols, n);
    if (type == 'T') cut_tsv (line, nums, n);
  }
}

char *usage = 
  "xcut 3 2 17        ... print columns 3, 2, 17 from TSV on stdin\n"
  "xcut age size type ... use 1st line to map age -> column number\n"
  "xcut age size type ... stdin = {JSON} records one-per-line\n"
  ;

int main (int argc, char *argv[]) {
  if (argc < 2) return fprintf (stderr, "\n%s\n", usage);
  cut_stdin(argv+1,argc-1);
  return 0;
}
