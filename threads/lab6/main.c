#define _GNU_SOURCE

#include <linux/futex.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include <syscall.h>

typedef struct {
    int id;
    void* (*fn)(void*);
    void* arg;
    void* retval;
    void* stack;

    int futex;
    char is_finished;
    char is_joined;

} mythread_struct;

typedef mythread_struct* mythread_t;


int mythread_start(void* arg) {
    mythread_t mythread = (mythread_t)arg;


    printf("in start\n");
    mythread->is_finished = 0;
    mythread->retval = mythread->fn(mythread->arg);
    mythread->is_finished = 1;

    syscall(SYS_futex, &mythread->futex, FUTEX_WAKE, 1, NULL, NULL, 0);

    while (!mythread->is_joined) {
        syscall(SYS_futex, &mythread->futex, FUTEX_WAIT, 0, NULL, NULL, 0);
    }

    return 0;
}

int mythread_create(mythread_t* tid, void*(*fn)(void* arg), void* arg) {

    static int ids = 0;
    ids++;

    void* region = mmap(NULL, getpagesize()*4, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0 );
    if (region == MAP_FAILED) {
      perror("mmap error");
      ids--;
      return -1;
    }

    mythread_t new_mythread = (mythread_t)((char*)region + getpagesize()*4 - sizeof(mythread_t));
    new_mythread->id = ids;
    new_mythread->fn = fn;
    new_mythread->arg = arg;
    new_mythread->retval = NULL;
    new_mythread->stack = region;

    new_mythread->futex = 0;
    new_mythread->is_finished = 0;
    new_mythread->is_joined = 0;

    const int flags = CLONE_VM | CLONE_SIGHAND | CLONE_FILES | CLONE_FS | CLONE_THREAD | CLONE_PARENT_SETTID | CLONE_SYSVSEM;

    printf("before clone\n");

    void* stack = (void*)new_mythread;

    int pid = clone(mythread_start, stack, flags, new_mythread);

    if (pid == -1) {
        perror("clone error");
        munmap(region, getpagesize()*4);
        ids--;
        return -1;
    }

    printf("after clone\n");

    *tid = new_mythread;

    return 0;
}

int mythread_join(mythread_t* tid, void** retval) {

    printf("in join\n");
    while (!(*tid)->is_finished) {
        syscall(SYS_futex, &(*tid)->futex, FUTEX_WAIT, 1, NULL, NULL, 0);
    }

    printf("in join\n");

    (*tid)->is_joined = 1;
    if (retval != NULL) {
        *retval = (*tid)->retval;
    }

    (*tid)->futex = 0;
    syscall(SYS_futex, &(*tid)->futex, FUTEX_WAKE, 0, NULL, NULL, 0);
    munmap((*tid)->stack, getpagesize()*4);
    return 0;
}

void* test(void* arg) {

    for (int i = 0; i < 10; i++) {
        printf("%d hello\n", i);
        sleep(1);
    }

    return NULL;
}


int main(void){

    mythread_t tid;

    mythread_create(&tid, test, NULL);

    mythread_join(&tid, NULL);

    return 0;
}
