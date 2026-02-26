#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include "synq.c"

// ---------- unit tests ----------

#define NUM_THREADS 8
#define INCREMENTS 100000

typedef struct {
  volatile int *lk;
  int *counter;
  int increments;
} lock_test_t;

void *lock_test_worker (void *arg) {
  lock_test_t *t = (lock_test_t *)arg;
  int i;
  for (i = 0; i < t->increments; i++) {
    lock (t->lk);
    (*t->counter)++;
    unlock (t->lk);
  }
  return NULL;
}

void test_lock () {
  volatile int lk = 0;
  int counter = 0;
  lock_test_t args [NUM_THREADS];
  pthread_t threads [NUM_THREADS];
  int i;
  for (i = 0; i < NUM_THREADS; i++) {
    args[i] = (lock_test_t) { &lk, &counter, INCREMENTS };
    pthread_create (&threads[i], NULL, lock_test_worker, &args[i]);
  }
  for (i = 0; i < NUM_THREADS; i++)
    pthread_join (threads[i], NULL);
  int expected = NUM_THREADS * INCREMENTS;
  fprintf (stderr, "lock test: counter=%d expected=%d %s\n",
           counter, expected, (counter == expected) ? "PASS" : "FAIL");
  assert (counter == expected);
}

int main (int argc, char *argv[]) {
  if (argc > 1 && !strcmp (argv[1], "-test-lock")) test_lock();
  else fprintf (stderr, "usage: test_synq -test-lock\n");
  return 0;
}
