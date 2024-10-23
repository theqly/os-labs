#include "uthread.h"
#include <stdio.h>

#include <unistd.h>

void * uthread_func1() {

  for (int i = 0; i < 3; i++) {
    printf("hello from thread1 %d\n", i);
    schedule();
    sleep(1);
  }

  return NULL;
}

void * uthread_func2() {

  for (int i = 0; i < 3; i++) {
    printf("hello from thread2 %d\n", i);
    schedule();
    sleep(1);
  }

  return NULL;
}


int main(void) {

  scheduler_init();

  uthread_t tid1, tid2;

  if(uthread_create(&tid1, uthread_func1, NULL) != 0) {
    printf("uthread_create failed\n");
    return -1;
  }

  if(uthread_create(&tid2, uthread_func2, NULL) != 0) {
    printf("uthread_create failed\n");
    return -1;
  }

  for (int i = 0; i < 3; i++) {
    printf("hello from main %d\n", i);
    schedule();
    sleep(1);
  }

  scheduler_destroy();

  return 0;
}
