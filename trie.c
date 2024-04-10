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

#define UP(i) ((uint)((i)/2))
#define DOWN(i) (2*(i))
#define RIGHT(i) (2*(i)+1)

inline char *down(char *trie, char *p) {

}

char *trie_ins (char *T, uint *N, char char *word) {
  uint i=1;

  for (w = word; *w; ++w) {
    while (T[i] && T[i] != *w) i = RIGHT(i); // find a place for *w
    if (!T[i]) T[i] = *w; // found an empty slot => insert
    i = DOWN(i);

    if (T[i] == 0) { T[i] = *w; i = DOWN(i); }
    if (T[i] == *w) i = DOWN(i);
    else

  }


}
