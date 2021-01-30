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

#ifndef QUERY
#define QUERY

typedef struct {
  char bool; // &:and +:or -:not
  char type; // ~:synonyms ":exact *:edits <>=:threshold
  float thr; // threshold value
  char *term; // query term
  uint id;    // id (once determined)
  ix_t *invl; // matching docs (once found)
  void *children; // for nested queries
} qry_t;

#endif
