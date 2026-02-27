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

#ifndef SYNQ_H
#define SYNQ_H

#include <stdatomic.h>
#include "types.h"

// ---------- lock-free FIFO queue ----------

typedef struct {
  _Atomic uint seq;
  _Atomic(void *) data;
} slot_t;

typedef struct {
  slot_t *items;
  uint size;
  _Atomic uint head;
  _Atomic uint tail;
} synq_t;

synq_t *synq_new  (uint n) ;
void    synq_free (synq_t *q) ;
void   *synq_push (synq_t *q, void *item) ;
void   *synq_pop  (synq_t *q) ;
uint    synq_len  (synq_t *q) ;

// ---------- thread utilities ----------

void   *detach (void *(*handle) (void *), void *arg) ;
int     busy   (volatile int *x) ;
void    lock   (volatile int *x) ;
void    unlock (volatile int *x) ;

// ---------- parallel map ----------

void tqdm (uint done, uint total, char *msg) ;
void pmap (uint nt, void *(*fn)(void *), void **in, void **out, uint n) ;
int  parallel (uint nt, uint n, int (*fn)(uint, void*), void *arg, char *msg) ;

// ---------- worker pool ----------

typedef struct {
  synq_t *in;
  synq_t *out;
  void *(*fn)(void *);
  _Atomic int stop;
} pool_t;

pool_t *new_pool  (uint nt, synq_t *in, synq_t *out, void *(*fn)(void *)) ;
void    stop_pool (pool_t *p) ;

#endif
