#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>

int global_var = 0;

void *mythread(void *arg) {
    int num = *((int*)arg);
    int local_var = 0;
    const int const_var = 0;
    static int static_var = 0;

    printf("*** THREAD %d *** mythread [%d %d %d]: Hello from mythread!\n", num, getpid(), getppid(), gettid());
    printf("*** THREAD %d *** thread ID (pthread_self): %lu, thread ID (gettid): %d\n", num, pthread_self(), gettid());

    printf("*** THREAD %d *** address of global_var: %p\n", num, (void*)&global_var);
    printf("*** THREAD %d *** address of local_var: %p\n", num, (void*)&local_var);
    printf("*** THREAD %d *** address of const_var: %p\n", num, (void*)&const_var);
    printf("*** THREAD %d *** address of static_var: %p\n", num, (void*)&static_var);

    local_var++;
    static_var++;
    global_var++;
    printf("*** THREAD %d *** local: %d, static: %d global: %d\n", num, local_var, static_var, global_var);

    sleep(999);

    pthread_exit(NULL);
}

int main() {
    pthread_t tids[5];
    int nums[5];
    int err;

    printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

    for (int i = 0; i < 2; i++) {
        nums[i] = i;
        err = pthread_create(&tids[i], NULL, mythread, &nums[i]);
        if (err) {
            printf("main: pthread_create() failed: %s\n", strerror(err));
            return -1;
        }
        printf("thread %i has tid %lu\n", i, tids[i]);
    }

    sleep(1000);

    for (int i = 0; i < 1; i++) {
        pthread_join(tids[i], NULL);
    }

    return 0;
}
