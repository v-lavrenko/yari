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

// http://ascii-table.com/ansi-escape-sequences.php
#define ESC        "\033["
#define CLS        ESC"2J"ESC"1;1H"
#define RESET      ESC"0m"
#define BOLD       ESC"1m"
#define NORMAL     ESC"2m"
#define UNDER      ESC"4m"
#define BLINK      ESC"5m"
#define INVERSE    ESC"7m"
#define fg_BLACK   ESC"30m"
#define fg_RED     ESC"31m"
#define fg_GREEN   ESC"32m"
#define fg_YELLOW  ESC"33m"
#define fg_BLUE    ESC"34m"
#define fg_MAGENTA ESC"35m"
#define fg_CYAN    ESC"36m"
#define fg_WHITE   ESC"37m"
#define bg_BLACK   ESC"40m"
#define bg_RED     ESC"41m"
#define bg_GREEN   ESC"42m"
#define bg_YELLOW  ESC"43m"
#define bg_BLUE    ESC"44m"
#define bg_MAGENTA ESC"45m"
#define bg_CYAN    ESC"46m"
#define bg_WHITE   ESC"47m"


char *color (char c) {
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
  case '!': return BLINK;
    //case 'r': return CLS;
    //case 'r': return RESET;
  }
  return "";
}

char *usage = 
  "hl 'Y:string' ... highlight in red any line of stdin that contains 'string'\n"
  "                  R:red G:green B:blue C:cyan M:magenta Y:yellow W:white K:black\n"
  "                  lowercase:foreground, *:bold _:underline ~:inverse !:blink\n"
  ;


int main (int argc, char *argv[]) {
  char line[1000000], **a, **end = argv + argc;
  if (argc < 2) return fprintf (stderr, "\n%s\n", usage);
  while (fgets (line, 999999, stdin)) {
    char *eol = index (line,'\n'), *clr = "";
    if (eol) *eol = 0;
    for (a = argv; a < end; ++a) if (strstr(line,*a+2)) clr = color(**a);
    printf ("%s%s%s\n", clr, line, ((*clr)?RESET:""));
  }
  return 1;
}
