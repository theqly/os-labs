#define _GNU_SOURCE
#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>

void sigint_handler(int s) {
    printf("thread 2 received signal %d\n", s);
}

void* mythread1(void* arg) {

    printf("thread 1 has tid : %d\n", gettid());

    sigset_t set;
    sigfillset(&set);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    printf("thread 1 blocked all signals\n");

    while (1) {
        sleep(1);
    }

    pthread_exit(NULL);
}

void* mythread2(void* arg) {

    printf("thread 2 has tid : %d\n", gettid());

    signal(SIGINT, sigint_handler);

    printf("thread 2 waiting for sigint\n");

    while (1) {
        sleep(1);
    }

    pthread_exit(NULL);
}

void* mythread3(void* arg) {

    printf("thread 3 has tid : %d\n", gettid());

    sigset_t set;
    int sig;

    sigemptyset(&set);
    sigaddset(&set, SIGQUIT);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    sigwait(&set, &sig);
    if (sig == SIGQUIT) {
        printf("thread 3 received sigquit\n");
    }

    pthread_exit(NULL);
}

int main() {
    pthread_t thread1, thread2, thread3;

    pthread_create(&thread1, NULL, mythread1, NULL);
    pthread_create(&thread2, NULL, mythread2, NULL);
    pthread_create(&thread3, NULL, mythread3, NULL);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);

    return 0;
}
