#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>

int global_var = 0;

void *mythread(void *arg) {
    int num = *((int*)arg);
    int local_var = 1;
    const int const_var = 2;
    static int static_var = 3;

    printf("*** THREAD %d *** mythread [%d %d %d]: Hello from mythread!\n", num, getpid(), getppid(), gettid());
    printf("*** THREAD %d *** thread ID (pthread_self): %lu, thread ID (gettid): %ld\n", num, pthread_self(), syscall(SYS_gettid));

    printf("*** THREAD %d *** address of global_var: %p\n", num, (void*)&global_var);
    printf("*** THREAD %d *** address of local_var: %p\n", num, (void*)&local_var);
    printf("*** THREAD %d *** address of const_var: %p\n", num, (void*)&const_var);
    printf("*** THREAD %d *** address of static_var: %p\n", num, (void*)&static_var);

    return NULL;
}

int main() {
    pthread_t tids[5];
    int err;

    printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

    for (int i = 0; i < 2; i++) {
        err = pthread_create(&tids[i], NULL, mythread, (void*)&i);
        if (err) {
            printf("main: pthread_create() failed: %s\n", strerror(err));
            return -1;
        }
        printf("thread %i has tid %lu\n", i, tids[i]);
    }

    for (int i = 0; i < 1; i++) {
        pthread_join(tids[i], NULL);
    }

    return 0;
}
