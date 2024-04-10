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

#include "vector.h"

#ifndef ORDERING
#define ORDERING

//typedef struct {
//  uint prev; //
//  uint next; //
//} ord_t; // doubly-linked list

// remove id from the list
void ord_del (ord_t *L, uint id) {
  if (!id || id >= len(L)) return; // id out of range
  ord_t *this = L + id, *next = L + this->next, *prev = L + this->prev;
  next->prev = this->prev; // previous <- next
  prev->next = this->next; // previous -> next
  this->prev = this->next = 0; // disconnect id
}

// insert id into list before 'next_id' (0 => last)
ord_t *ord_ins (ord_t *L, uint id, uint next_id) {
  if (id+1 >= len(L)) L = resize_vec (L, id+1);
  ord_t *this = L + id, *next = L + next_id, *prev = L + next->prev;
  this->next = next_id;           //         this -> next
  this->prev = next->prev;        // prev <- this
  next->prev = id;                //         this <- next
  prev->next = id;                // prev -> this
  return L;
}

/*
inline uint ord_next (ord_t *L, uint id) {
  return L ? L[id].next : (id+1 < n) ? (id+1) : 0;
}
*/

// instantiate a new sequential linked list
ord_t *ord_init (ord_t *L, uint n) {
  uint i;
  L = resize_vec (L, n+1);
  for (i = 1; i < n; ++i) { L[i].next = i+1; L[i].prev = i-1; }
  L[0].next = 1; L[0].prev = n;
  L[n].next = 0; L[n].prev = n-1;
  return L;
}

#endif
