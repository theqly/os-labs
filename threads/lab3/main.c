#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

struct foo {
    char* b;
    int a;
};


void *mythread(void *arg) {
    struct foo *bar = arg;
    printf("a: %d, b: %s\n", bar->a, bar->b);
    pthread_exit(NULL);
}

int main() {
    pthread_t tid;
    pthread_attr_t attr;

    struct foo *bar = malloc(sizeof(struct foo));
    bar->a = 5;
    bar->b = "hello";

    printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

    int ret = pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    int err = pthread_create(&tid, &attr, mythread, bar);
    if (err) {
        printf("main: pthread_create() failed: %s\n", strerror(err));
        return -1;
    }

    pthread_join(tid, NULL);

    sleep(3);
    free(bar);

    return 0;
}
