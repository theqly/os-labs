#ifndef UTHREAD_H
#define UTHREAD_H
#include <ucontext.h>

#define STACK_SIZE 4096
#define MAX_THREADS 16

typedef struct {
  void* (*fn)(void*);
  void* arg;
  ucontext_t context;
} uthread_struct;

typedef uthread_struct *uthread_t;

int uthread_create(uthread_t* thread, void* (*fn)(void*), void* arg);

void* uthread_start(void* arg);

typedef struct {
  uthread_t threads[MAX_THREADS];
  unsigned int thread_count;
  unsigned int cur_thread;
} scheduler;

void scheduler_init();

void schedule();

void scheduler_destroy();

#endif //UTHREAD_H
