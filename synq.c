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
#include <sched.h>
#include <stdatomic.h>

extern ulong next_pow2 (ulong x);

// ---------- lock-free multi-reader / multi-writer FIFO queue ----------
//
// example:
//   synq_t *Q = synq_new (1024);
//   ok = synq_push (Q, item); 
//   item = synq_pop (Q);
//   synq_free (Q);

typedef struct {
  _Atomic uint seq;     // what phase this slot is in
  _Atomic(void *) data; // item stored in this slot
} slot_t;

typedef struct {
  slot_t *items; // circular buffer of slots
  uint size;     // capacity (power of 2)
  _Atomic uint head; // next index to dequeue from
  _Atomic uint tail; // next index to enqueue at
} synq_t;

synq_t *synq_new (uint n) {
  synq_t *q = calloc (1, sizeof (synq_t));
  q->size = next_pow2 (n);
  q->items = calloc (q->size, sizeof (slot_t));
  for (uint i = 0; i < q->size; ++i)
    atomic_init (&q->items[i].seq, i); // slot i ready for write #i
  atomic_init (&q->head, 0);
  atomic_init (&q->tail, 0);
  return q;
}

void synq_free (synq_t *q) {
  free (q->items);
  free (q);
}

// adds item to the tail of the queue, returns NULL if full
void *synq_push (synq_t *q, void *item) {
  assert (q && item);
  uint tail = atomic_load (&q->tail);
  for (;;) {
    slot_t *s = &q->items[tail % q->size];
    uint seq = atomic_load_explicit (&s->seq, memory_order_acquire);
    int diff = (int)seq - (int)tail;
    if (diff == 0) { // slot is ready for this write
      if (atomic_compare_exchange_weak (&q->tail, &tail, tail + 1)) {
        atomic_store_explicit (&s->data, item, memory_order_relaxed);
        atomic_store_explicit (&s->seq, tail + 1, memory_order_release);
        return item;
      }
    } else if (diff < 0) {
      return NULL; // full
    } else {
      tail = atomic_load (&q->tail); // another thread advanced tail
    }
  }
}

// removes item from the head of the queue, returns NULL if empty
void *synq_pop (synq_t *q) {
  assert (q);
  uint head = atomic_load (&q->head);
  for (;;) {
    slot_t *s = &q->items[head % q->size];
    uint seq = atomic_load_explicit (&s->seq, memory_order_acquire);
    int diff = (int)seq - (int)(head + 1);
    if (diff == 0) { // slot has data for this read
      if (atomic_compare_exchange_weak (&q->head, &head, head + 1)) {
        void *item = atomic_load_explicit (&s->data, memory_order_relaxed);
        atomic_store_explicit (&s->seq, head + q->size, memory_order_release);
        return item;
      }
    } else if (diff < 0) {
      return NULL; // empty
    } else {
      head = atomic_load (&q->head); // another thread advanced head
    }
  }
}

uint synq_len (synq_t *q) {
  return atomic_load (&q->tail) - atomic_load (&q->head);
}

// -------------------------- thread-related --------------------------

void *detach (void *(*handle) (void *), void *arg) {
  pthread_t t = 0;
  if (pthread_create (&t, NULL, handle, arg)) assert (0);
  if (pthread_detach (t)) assert (0);
  return NULL;
}

int busy (volatile int *x) { // 1: item is busy, 0: we locked it
  return !__sync_bool_compare_and_swap (x, 0, 1); // !true if swapped 0 -> 1
}

void lock (volatile int *x) { while (busy (x)) sched_yield(); }

void unlock (volatile int *x) { __sync_lock_release (x); }

// -------------------------- parallel map --------------------------

typedef struct {
  void **in;           // input array
  void **out;          // output array
  void *(*fn)(void *); // handler
  _Atomic uint next;   // next index to process
  uint n;              // total number of inputs
} pmap_t;

void *pmap_worker (void *arg) {
  pmap_t *p = (pmap_t *)arg;
  for (;;) {
    uint i = atomic_fetch_add (&p->next, 1);
    if (i >= p->n) break;
    p->out[i] = p->fn (p->in[i]);
  }
  return NULL;
}

void **parallel (uint nt, void *(*fn)(void *), void **in, uint n) {
  void **out = calloc (n, sizeof (void*));
  pmap_t ctx = { in, out, fn, 0, n };
  atomic_init (&ctx.next, 0);
  pthread_t *threads = calloc (nt, sizeof (pthread_t));
  for (uint i = 0; i < nt; ++i)
    pthread_create (&threads[i], NULL, pmap_worker, &ctx);
  for (uint i = 0; i < nt; ++i)
    pthread_join (threads[i], NULL);
  free (threads);
  return out;
}

#ifdef MAIN

int main (int argc, char *argv[]) {
  (void) argc;
  (void) argv;
  return 0;
}

#endif
