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
#ifndef REGEXP
#define REGEXP

// compile pattern into regex_t
regex_t re_compile(char *pattern, char *prm);
// beg:end for all sub-elements of the 1st match of RE in text
it_t *re_find_els(regex_t RE, char *text) ;
// beg:end for all matches of the entire RE in text
it_t *re_find_all(regex_t RE, char *text) ;
// de-allocate regex_t
void re_free(regex_t RE) ;

#endif
