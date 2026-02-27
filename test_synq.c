#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include "synq.c"

#define PASS "✅ PASS"
#define FAIL "❌ FAIL"

// ==================== lock/unlock tests ====================

#define LOCK_THREADS 8
#define LOCK_ITERS 100000

typedef struct {
  volatile int *lk;
  int *counter;
  int iters;
} lock_test_t;

void *lock_test_worker (void *arg) {
  lock_test_t *t = (lock_test_t *)arg;
  int i;
  for (i = 0; i < t->iters; i++) {
    lock (t->lk);
    (*t->counter)++;
    unlock (t->lk);
  }
  return NULL;
}

void test_lock () {
  volatile int lk = 0;
  int counter = 0;
  lock_test_t args [LOCK_THREADS];
  pthread_t threads [LOCK_THREADS];
  int i;
  for (i = 0; i < LOCK_THREADS; i++) {
    args[i] = (lock_test_t) { &lk, &counter, LOCK_ITERS };
    pthread_create (&threads[i], NULL, lock_test_worker, &args[i]);
  }
  for (i = 0; i < LOCK_THREADS; i++)
    pthread_join (threads[i], NULL);
  int expected = LOCK_THREADS * LOCK_ITERS;
  fprintf (stderr, "lock test: counter=%d expected=%d %s\n",
           counter, expected, (counter == expected) ? PASS : FAIL);
  assert (counter == expected);
}

// ==================== synq push/pop tests ====================

// -- basic single-threaded test --

void test_synq_basic () {
  synq_t *q = synq_new (4); // capacity rounds up to 4
  int a = 1, b = 2, c = 3, d = 4, e = 5;

  // push until full
  assert (synq_push (q, &a));
  assert (synq_push (q, &b));
  assert (synq_push (q, &c));
  assert (synq_push (q, &d));
  assert (!synq_push (q, &e)); // full

  // pop in FIFO order
  assert (synq_pop (q) == &a);
  assert (synq_pop (q) == &b);
  assert (synq_pop (q) == &c);
  assert (synq_pop (q) == &d);
  assert (synq_pop (q) == NULL); // empty

  // push again after draining
  assert (synq_push (q, &e));
  assert (synq_pop (q) == &e);

  synq_free (q);
  fprintf (stderr, "synq basic test: "PASS"\n");
}

// -- multi-threaded producer/consumer test --

#define SYNQ_ITEMS 100000
#define SYNQ_PRODUCERS 4
#define SYNQ_CONSUMERS 4

typedef struct {
  synq_t *q;
  int start; // first item id
  int count; // number of items to push
} producer_arg_t;

typedef struct {
  synq_t *q;
  long *sum;             // shared accumulator
  volatile int *done;    // set to 1 when all producers are done
  volatile int *sum_lock;
} consumer_arg_t;

void *producer_worker (void *arg) {
  producer_arg_t *p = (producer_arg_t *)arg;
  int i;
  for (i = 0; i < p->count; i++) {
    ulong val = (ulong)(p->start + i + 1); // +1 so no NULL pointers
    while (!synq_push (p->q, (void*)val))
      sched_yield(); // queue full, retry
  }
  return NULL;
}

void *consumer_worker (void *arg) {
  consumer_arg_t *c = (consumer_arg_t *)arg;
  long local_sum = 0;
  for (;;) {
    void *item = synq_pop (c->q);
    if (item) {
      local_sum += (ulong)item;
    } else if (*c->done) {
      // producers done and queue empty: try one more time
      item = synq_pop (c->q);
      if (item) local_sum += (ulong)item;
      else break;
    } else {
      sched_yield();
    }
  }
  lock (c->sum_lock);
  *c->sum += local_sum;
  unlock (c->sum_lock);
  return NULL;
}

void test_synq_concurrent () {
  synq_t *q = synq_new (256);
  int total = SYNQ_ITEMS;
  int per_producer = total / SYNQ_PRODUCERS;
  volatile int done = 0;
  volatile int sum_lock = 0;
  long sum = 0;

  // expected sum: sum of 1..total
  long expected = (long)total * (total + 1) / 2;

  pthread_t prod [SYNQ_PRODUCERS];
  pthread_t cons [SYNQ_CONSUMERS];
  producer_arg_t pargs [SYNQ_PRODUCERS];
  consumer_arg_t carg = { q, &sum, &done, &sum_lock };

  int i;
  // start consumers first
  for (i = 0; i < SYNQ_CONSUMERS; i++)
    pthread_create (&cons[i], NULL, consumer_worker, &carg);
  // start producers
  for (i = 0; i < SYNQ_PRODUCERS; i++) {
    pargs[i] = (producer_arg_t) { q, i * per_producer, per_producer };
    pthread_create (&prod[i], NULL, producer_worker, &pargs[i]);
  }
  // wait for producers
  for (i = 0; i < SYNQ_PRODUCERS; i++)
    pthread_join (prod[i], NULL);
  __sync_lock_test_and_set (&done, 1);
  // wait for consumers
  for (i = 0; i < SYNQ_CONSUMERS; i++)
    pthread_join (cons[i], NULL);

  fprintf (stderr, "synq concurrent test: sum=%ld expected=%ld %s\n",
           sum, expected, (sum == expected) ? PASS : FAIL);
  assert (sum == expected);
  synq_free (q);
}

// ==================== parallel tests ====================

void *triple (void *x) { return (void*)((ulong)x * 3); }

void test_parallel_basic () {
  uint n = 10;
  void **in = calloc (n, sizeof (void*));
  for (uint i = 0; i < n; ++i)
    in[i] = (void*)(ulong)(i + 1); // 1..10
  void **out = parallel (4, triple, in, n);
  for (uint i = 0; i < n; ++i)
    assert ((ulong)out[i] == (i + 1) * 3);
  free (in);
  free (out);
  fprintf (stderr, "parallel basic test: "PASS"\n");
}

void test_parallel_stress () {
  uint n = 100000000;
  void **in = calloc (n, sizeof (void*));
  for (uint i = 0; i < n; ++i)
    in[i] = (void*)(ulong)(i + 1); // 1..n
  void **out = parallel (48, triple, in, n);
  long sum = 0;
  for (uint i = 0; i < n; ++i)
    sum += (ulong)out[i];
  long expected = 3L * (long)n * (n + 1) / 2;
  fprintf (stderr, "parallel stress test: sum=%ld expected=%ld %s\n",
           sum, expected, (sum == expected) ? PASS : FAIL);
  assert (sum == expected);
  free (in);
  free (out);
}

// ==================== main ====================

int main (int argc, char *argv[]) {
  if (argc < 2) {
    fprintf (stderr, "usage: test_synq -test-lock | -test-synq | -test-parallel | -test-all\n");
    return 1;
  }
  if (!strcmp (argv[1], "-test-lock") || !strcmp (argv[1], "-test-all"))
    test_lock();
  if (!strcmp (argv[1], "-test-synq") || !strcmp (argv[1], "-test-all")) {
    test_synq_basic();
    test_synq_concurrent();
  }
  if (!strcmp (argv[1], "-test-parallel") || !strcmp (argv[1], "-test-all")) {
    test_parallel_basic();
    test_parallel_stress();
  }
  return 0;
}
