#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

void* mythread(void* arg) {
    int counter = 0;

    char* str = malloc(20);
    strcpy(str, "bebebe\n");

    pthread_cleanup_push(free, str);

    while (1) {
        //printf("hello\n");
        //counter;
        printf("%s", str);
        pthread_testcancel();
    }

    pthread_cleanup_pop(1);

}


int main(void) {
    printf("start\n");
    pthread_t tid;

    if (pthread_create(&tid, NULL, mythread, NULL)) {
        fprintf(stderr, "main: pthread_create() failed\n");
        return -1;
    }

    sleep(1);
    pthread_cancel(tid);

    pthread_join(tid, NULL);

    return 0;
}
