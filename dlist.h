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
