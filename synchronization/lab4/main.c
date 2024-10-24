#include "mymutex.h"
#include "myspinlock.h"

#include <stdio.h>
#include <pthread.h>


int shared_counter = 0;
mymutex mtx;
myspinlock spinlock;

void *thread_func(void *arg) {
    for (int i = 0; i < 100000; i++) {
        //mymutex_lock(&mtx);
        myspinlock_lock(&spinlock);
        shared_counter++;
        //mymutex_unlock(&mtx);
        myspinlock_unlock(&spinlock);
    }
    return NULL;
}

int main() {
    pthread_t threads[10];

    //mymutex_init(&mtx);
    myspinlock_init(&spinlock);

    for (int i = 0; i < 10; i++) {
        if (pthread_create(&threads[i], NULL, thread_func, NULL) != 0) {
            perror("pthread_create");
            return -1;
        }
    }

    for (int i = 0; i < 10; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("res: %d\n", shared_counter); //nado 1000000

    return 0;
}
