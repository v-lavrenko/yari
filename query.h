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
  char *tok; // token behind query term
  char op; // &:and +:or -:not x:skip
  char type; // ~:synonyms ":exact *:edits <>=:threshold x:skip
  float thr; // threshold value
  uint id;   // id (once determined)
  uint id2;  // if token is a run-on
  ix_t *docs; // matching docs (once found)
  //void *children; // for nested queries
} qry_t;

qry_t *str2qry (char *str) ; // 'amino -acid +"fab"' -> qry_t[3]
char  *qry2str (qry_t *Q, hash_t *H) ; // Q -> 'amino -acid +"fab"'
void  free_qry (qry_t *Q) ;
void spell_qry (qry_t *Q, hash_t *H, float *F, char *prm) ;
ix_t *exec_qry (qry_t *Q, hash_t *H, coll_t *INVL) ;

#endif
