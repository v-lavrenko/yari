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
#include <string.h>
#include <assert.h>
#include "textutil.h"

char *usage = 
  "hl 'Y:string' ... highlight in red any line of stdin that contains 'string'\n"
  "                  R:red G:green B:blue C:cyan M:magenta Y:yellow W:white K:black\n"
  "                  lowercase:foreground, *:bold _:underline ~:inverse #:blink\n"
  ;


int *col_offs (char *line) {
  int n = 1; 
  for (s = line; *s; ++s) if (*s == tab) ++n; // count tabs
  int nums = calloc(n,sizeof(int))
}

int *col_nums (char **cols, int n) {
  int *nums = calloc(n,sizeof(int)), i;
  for (i=0; i<n; ++i) nums[i] = atoi(cols[i]);
  return nums;
}

int *col_names (char **cols, int n, char *hdr) {
  int *nums = calloc(n,sizeof(int)), i, j;
  hdr += strspn(hdr," #"); // skip initial
  for (i=0; i<n; ++i) {
    
  }
}

void cut_col_num (char *line, char **cols, int n) {
  
  char *tsv_value (char *line, uint col)
  
}



static void hl_line(char *line, char **hl, int n) {
  char *clr = NULL, **h = NULL;
  for (h = hl; h < hl+n; ++h)
    if (strstr(line,*h+2)) clr = hl_color(**h);
  if (clr) fputs(clr, stdout);   // start color
  fputs(line,stdout);            // full line
  if (clr) fputs(RESET, stdout); // end color
  fputc('\n',stdout);
  //printf ("%s%s%s\n", clr, line, ((*clr)?RESET:""));  
}

void hl_subs(char *line, char *h) {
  int l = strlen(h)-2; assert(l>0);
  while (1) {
    char *beg = strstr(line,h+2);
    if (!beg) break;
    fwrite(line,beg-line,1,stdout); // part before match
    fputs(hl_color(*h), stdout);    // start color 
    fputs(h+2, stdout);             // match itself
    fputs(RESET, stdout);           // end color
    line = beg + l;                 // part after match
  }
  fputs(line,stdout);
  fputc('\n',stdout);
}

int main (int argc, char *argv[]) {
  char line[1000000];
  if (argc < 2) return fprintf (stderr, "\n%s\n", usage);
  while (fgets (line, 999999, stdin)) {
    char *eol = index (line,'\n');
    if (eol) *eol = 0;
    if (argv[1][1] == '@') hl_subs (line, argv[1]); // highlight substrings
    else hl_line (line, argv, argc); // highlight whole lines
  }
  return 0;
}
