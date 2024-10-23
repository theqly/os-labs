#include "uthread.h"

#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>

scheduler sched;

int uthread_create(uthread_t* utid, void* (*fn)(void*), void* arg) {
  void* region = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if( region == MAP_FAILED ) {
    perror("mmap error");
    return -1;
  }

  uthread_t new_thread = (uthread_t)((char*)region + STACK_SIZE - sizeof(uthread_struct));

  int err = getcontext(&new_thread->context);
  if( err == -1 ) {
    perror("getcontext error");
    return -1;
  }

  new_thread->context.uc_stack.ss_sp = region;
  new_thread->context.uc_stack.ss_size = STACK_SIZE - sizeof(uthread_t);
  new_thread->context.uc_link = NULL;
  makecontext(&new_thread->context, (void (*)(void))uthread_start, 1, (intptr_t)new_thread);

  new_thread->fn = fn;
  new_thread->arg = arg;

  sched.threads[sched.thread_count++] = new_thread;

  *utid = new_thread;

  return 0;
}

void* uthread_start(void* arg) {
  uthread_t utid = (uthread_t)(intptr_t)arg;


  utid->fn(utid->arg);

  return NULL;
}



void scheduler_init() {
  sched.thread_count = 0;
  sched.cur_thread = 0;

  uthread_t main_thread = malloc(sizeof(uthread_struct));
  if (getcontext(&(main_thread->context)) == -1) {
    perror("getcontext error for main thread");
    exit(1);
  }

  sched.threads[sched.thread_count++] = main_thread;
}

void schedule() {

  ucontext_t* current_context = &(sched.threads[sched.cur_thread]->context);
  sched.cur_thread = (sched.cur_thread + 1) % sched.thread_count;
  ucontext_t* next_context = &(sched.threads[sched.cur_thread]->context);

  int err = swapcontext(current_context, next_context);
  if( err == -1 ) {
    perror("swapcontext error");
    exit(-1);
  }

}

void scheduler_destroy() {
  free(sched.threads[0]);
  for(int i = 1; i < sched.thread_count; i++) {
    munmap(sched.threads[i]->context.uc_stack.ss_sp, STACK_SIZE);
  }
}