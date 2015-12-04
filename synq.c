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

inline int busy (int *x) { // 1: item is busy, 0: we locked it
  return !__sync_bool_compare_and_swap (x, 0, 1); // !true if swapped 0 -> 1
}

inline void lock (int *x) { while (busy (x)) usleep (10); }

inline void unlock (int *x) { *x = 0; }
