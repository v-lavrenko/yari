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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "types.h"
#include <pthread.h>
#include <unistd.h>

extern ulong next_pow2 (ulong x);

// ---------- non-blocking multi-reader / multi-writer FIFO queue ----------

typedef struct {
  void **items; // circular fifo
  uint size; // #elements in the queue
  uint head; // index of current head
  uint tail; // index of current tail
} synq_t;

synq_t *synq_new (uint n) {
  synq_t *q = calloc (1, sizeof (synq_t));
  q->head = q->tail = 0;
  q->size = next_pow2 (n);
  q->items = calloc (q->size, sizeof (void*));
  return q;
}

void synq_free (synq_t *q) {
  memset (q->items, 0, q->size * sizeof(void*));
  free (q->items);
  memset (q, 0, sizeof(synq_t));
  free (q);
}

void *synq_push (synq_t *q, void *item) {
  assert (q && item);
  uint i = __sync_fetch_and_add (&(q->head), 1); // q->head++
  void **slot = q->items + i % q->size;
  if (!*slot) return (*slot = item);
  __sync_fetch_and_sub (&(q->head), 1); // q->head--
  return 0;
}

void *synq_pop (synq_t *q) {
  assert (q);
  uint i = __sync_fetch_and_add (&(q->tail), 1); // q->tail++
  void *item = q->items [i % q->size];
  if (item) q->items [i % q->size] = NULL;
  else __sync_fetch_and_sub (&(q->tail), 1); // q->tail--
  return item;
}

// -------------------------- thread-related --------------------------

void *detach (void *(*handle) (void *), void *arg) {
  pthread_t t = 0;
  if (pthread_create (&t, NULL, handle, arg)) assert (0);
  if (pthread_detach (t)) assert (0);
  return NULL;
}

int busy (int *x) { // 1: item is busy, 0: we locked it
  return !__sync_bool_compare_and_swap (x, 0, 1); // !true if swapped 0 -> 1
}

void lock (int *x) { while (busy (x)) usleep (10); }

void unlock (int *x) { *x = 0; }

// -------------------------- thread-related --------------------------

#ifdef MAIN
/*
int main (int argc, char *argv[]) {
(void) argc;
(void) argv;
return 0;
}
*/
#endif
