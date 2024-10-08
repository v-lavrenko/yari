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

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "hl.h"

char *hl_color (char c) {
  switch (c) {
  case 'k': return fg_BLACK;
  case 'r': return fg_RED;
  case 'g': return fg_GREEN;
  case 'y': return fg_YELLOW;
  case 'b': return fg_BLUE;
  case 'm': return fg_MAGENTA;
  case 'c': return fg_CYAN;
  case 'w': return fg_WHITE;
  case 'K': return bg_BLACK;
  case 'R': return bg_RED;
  case 'G': return bg_GREEN;
  case 'Y': return bg_YELLOW;
  case 'B': return bg_BLUE;
  case 'M': return bg_MAGENTA;
  case 'C': return bg_CYAN;
  case 'W': return bg_WHITE;
  case '*': return BOLD;
  case '_': return UNDER;
  case '~': return INVERSE;
  case '#': return BLINK;
    //case 'r': return CLS;
    //case 'r': return RESET;
  }
  return "";
}

//  "                  R:red G:green B:blue C:cyan M:magenta Y:yellow W:white K:black\n"
//  "                  lowercase:foreground, *:bold _:underline ~:inverse #:blink\n"
char *usage =
  "hl xml        ... highlight SGML tags\n"
  "hl 'Y:string' ... highlight in red any line of stdin that contains 'string'\n\t\t"
  " r:"fg_RED"red"RESET
  " g:"fg_GREEN"green"RESET
  " b:"fg_BLUE"blue"RESET
  " c:"fg_CYAN"cyan"RESET
  " m:"fg_MAGENTA"magenta"RESET
  " y:"fg_YELLOW"yellow"RESET
  " k:"fg_BLACK"black"RESET
  " w:"fg_WHITE"white"RESET"\n\t\t"
  " R:"bg_RED"red"RESET
  " G:"bg_GREEN"green"RESET
  " B:"bg_BLUE"blue"RESET
  " C:"bg_CYAN"cyan"RESET
  " M:"bg_MAGENTA"magenta"RESET
  " Y:"bg_YELLOW"yellow"RESET
  " K:"bg_BLACK"black"RESET
  " W:"bg_WHITE"white"RESET"\n\t\t"
  " *:"BOLD"bold"RESET
  " _:"UNDER"underline"RESET
  " #:"BLINK"blink"RESET
  " ~:"INVERSE"inverse"RESET"\n"
  ;

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

int hl_xml() {
  char *TAG = fg_GREEN, *STR = fg_CYAN;
  int c, prev = 0, intag = 0, instr = 0;
  while ((c = getchar()) != EOF) {
    if (c == '<' && prev != '\\' && !intag) { // opening <
      fputs(TAG, stdout);
      intag = 1;
      putchar(c);
    }
    else if (c == '"' && intag && !instr) { // opening " in <tag>
      fputs(STR, stdout);
      instr = 1;
      putchar(c);
    }
    else if (c == '"' && instr) { // closing "
      putchar(c);
      fputs(intag ? TAG : RESET, stdout);
      instr = 0;
    }
    else if (c == '>' && intag && !instr) { // closing >
      putchar(c);
      fputs(RESET,stdout);
      intag = 0;
    }
    else putchar(c);
    prev = c;
  }
  return 0;
}

int main (int argc, char *argv[]) {
  char line[1000000];
  if (argc < 2) return fprintf (stderr, "\n%s\n", usage);
  if (!strcmp(argv[1],"xml")) return hl_xml();
  while (fgets (line, 999999, stdin)) {
    char *eol = index (line,'\n');
    if (eol) *eol = 0;
    if (argv[1][1] == '@') hl_subs (line, argv[1]); // highlight substrings
    else hl_line (line, argv, argc); // highlight whole lines
    fflush(stdout);
  }
  return 0;
}
