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
    //case 'r': return CLS;
    //case 'r': return RESET;
  }
  return "";
}

int main (int argc, char *argv[]) {
  char line[1000000], **a, **end = argv + argc;
  while (fgets (line, 999999, stdin)) {
    char *eol = index (line,'\n'), *clr = "";
    if (eol) *eol = 0;
    for (a = argv; a < end; ++a) if (strstr(line,*a+2)) clr = color(**a);
    printf ("%s%s%s\n", clr, line, ((*clr)?RESET:""));
  }
  return 1;
}
