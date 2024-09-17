#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

void* mythread(void* arg) {
    int counter = 0;
    while (1) {
        //printf("hello\n");
        ++counter;
        sleep(1);
    }
    pthread_exit(NULL);
}


int main(void) {
    printf("Hello, World!\n");
    pthread_t tid;
    int err = pthread_create(&tid, NULL, mythread, NULL);
    if (err) {
        printf("main: pthread_create() failed: %s\n", strerror(err));
        return -1;
    }

    sleep(5);
    pthread_cancel(tid);


    return 0;
}
