#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>


void *mythread(void *arg) {
    printf("thread ID (pthread_self): %lu, thread ID (gettid): %d\n", pthread_self(), gettid());
    //pthread_detach(pthread_self());
    pthread_exit(NULL);
}

int main() {
    pthread_t tid;
    pthread_attr_t attr;

    printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

    int ret = pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    /*while(1) {
        int err = pthread_create(&tid, NULL, mythread, NULL);
        if (err) {
            printf("main: pthread_create() failed: %s\n", strerror(err));
            return -1;
        }
        pthread_join(tid, NULL);

        sleep(2);
    }*/

    int err = pthread_create(&tid, &attr, mythread, NULL);
    if (err) {
        printf("main: pthread_create() failed: %s\n", strerror(err));
        return -1;
    }

    pthread_join(tid, NULL);

    sleep(3);

    return 0;
}
