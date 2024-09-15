#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>


void *mythread(void *arg) {
    //int* x = malloc(sizeof(int));
    char* hello = "hello world";
    printf("mythread [%d %d %d]: Hello from mythread!\n", getpid(), getppid(), gettid());
    printf("thread ID (pthread_self): %lu, thread ID (gettid): %d\n", pthread_self(), gettid());

    pthread_exit(hello);
}

int main() {
    pthread_t tid;

    printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

    int err = pthread_create(&tid, NULL, mythread, NULL);
    if (err) {
        printf("main: pthread_create() failed: %s\n", strerror(err));
        return -1;
    }

    void* ret;

    pthread_join(tid, &ret);

    printf("%s\n", (char*)ret);

    //free(ret);

    return 0;
}
