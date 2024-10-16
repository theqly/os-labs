#include "list.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct  _Statistic {
    int inc_iters;
    int dec_iters;
    int eq_iters;
    int swap_attempts;

    pthread_spinlock_t inc_mutex;
    pthread_spinlock_t dec_mutex;
    pthread_spinlock_t eq_mutex;
    pthread_spinlock_t swap_mutex;
} Stat;

Stat stat = {};
Storage list;

void* search_inc(void *arg) {
    while (1) {
        Node *current = list.first;
        int count = 0;
        while (current && current->next) {
            pthread_mutex_lock(&current->sync);
            pthread_mutex_lock(&current->next->sync);

            if (strlen(current->value) < strlen(current->next->value)) {
                count++;
            }

            pthread_mutex_unlock(&current->next->sync);
            pthread_mutex_unlock(&current->sync);

            current = current->next;
        }
        pthread_spin_lock(&stat.inc_mutex);
        stat.inc_iters++;
        pthread_spin_unlock(&stat.inc_mutex);
    }
    return NULL;
}

void* search_dec(void *arg) {

    while (1) {
        Node *current = list.first;
        int count = 0;
        while (current && current->next) {
            pthread_mutex_lock(&current->sync);
            pthread_mutex_lock(&current->next->sync);

            if (strlen(current->value) > strlen(current->next->value)) {
                count++;
            }

            pthread_mutex_unlock(&current->next->sync);
            pthread_mutex_unlock(&current->sync);

            current = current->next;
        }
        pthread_spin_lock(&stat.dec_mutex);
        stat.dec_iters++;
        pthread_spin_unlock(&stat.dec_mutex);
    }
    return NULL;
}

void* search_eq(void *arg) {

    while (1) {
        Node *current = list.first;
        int count = 0;
        while (current && current->next) {
            pthread_mutex_lock(&current->sync);
            pthread_mutex_lock(&current->next->sync);

            if (strlen(current->value) == strlen(current->next->value)) {
                count++;
            }

            pthread_mutex_unlock(&current->next->sync);
            pthread_mutex_unlock(&current->sync);

            current = current->next;
        }
        pthread_spin_lock(&stat.eq_mutex);
        stat.eq_iters++;
        pthread_spin_unlock(&stat.eq_mutex);
    }
    return NULL;
}

void* random_swaps(void *arg) {
    while (1) {
        Node *current = list.first;
        while (current && current->next) {
            if (rand() % 2) {
                list_swap(current, current->next);
                pthread_spin_lock(&stat.swap_mutex);
                stat.swap_attempts++;
                pthread_spin_unlock(&stat.swap_mutex);
            }
            current = current->next;
        }
    }
    return NULL;
}

void* print_stat(void *arg) {

    while (1) {
        sleep(1);
        pthread_spin_lock(&stat.inc_mutex);
        pthread_spin_lock(&stat.dec_mutex);
        pthread_spin_lock(&stat.eq_mutex);
        pthread_spin_lock(&stat.swap_mutex);

        printf("inc iters: %d, dec iters: %d, eq iters: %d, swap attempts: %d\n", stat.inc_iters, stat.dec_iters, stat.eq_iters, stat.swap_attempts);

        pthread_spin_unlock(&stat.inc_mutex);
        pthread_spin_unlock(&stat.dec_mutex);
        pthread_spin_unlock(&stat.eq_mutex);
        pthread_spin_unlock(&stat.swap_mutex);
    }

    return NULL;
}

int main() {
    list_init(&list);

    list_insert(&list, "a");
    list_insert(&list, "bb");
    list_insert(&list, "ccc");
    list_insert(&list, "dddd");
    list_insert(&list, "eeeee");

    pthread_spin_init(&stat.inc_mutex, PTHREAD_PROCESS_PRIVATE);
    pthread_spin_init(&stat.dec_mutex, PTHREAD_PROCESS_PRIVATE);
    pthread_spin_init(&stat.eq_mutex, PTHREAD_PROCESS_PRIVATE);
    pthread_spin_init(&stat.swap_mutex, PTHREAD_PROCESS_PRIVATE);

    int err;
    pthread_t tid_inc, tid_dec, tid_eq;
    pthread_t tid_swap1, tid_swap2, tid_swap3;
    pthread_t tid_stat;

    err = pthread_create(&tid_stat, NULL, print_stat, NULL);
    if (err) {
        printf("main: pthread_create() failed: %s\n", strerror(err));
        return -1;
    }

    err = pthread_create(&tid_inc, NULL, search_inc, NULL);
    if (err) {
        printf("main: pthread_create() failed: %s\n", strerror(err));
        return -1;
    }
    err = pthread_create(&tid_dec, NULL, search_dec, NULL);
    if (err) {
        printf("main: pthread_create() failed: %s\n", strerror(err));
        return -1;
    }
    err = pthread_create(&tid_eq, NULL, search_eq, NULL);
    if (err) {
        printf("main: pthread_create() failed: %s\n", strerror(err));
        return -1;
    }

    err = pthread_create(&tid_swap1, NULL, random_swaps, NULL);
    if (err) {
        printf("main: pthread_create() failed: %s\n", strerror(err));
        return -1;
    }
    err = pthread_create(&tid_swap2, NULL, random_swaps, NULL);
    if (err) {
        printf("main: pthread_create() failed: %s\n", strerror(err));
        return -1;
    }
    err = pthread_create(&tid_swap3, NULL, random_swaps, NULL);
    if (err) {
        printf("main: pthread_create() failed: %s\n", strerror(err));
        return -1;
    }

    err = pthread_join(tid_inc, NULL);
    if (err) {
        printf("main: pthread_join() failed: %s\n", strerror(err));
        return -1;
    }
    err = pthread_join(tid_dec, NULL);
    if (err) {
        printf("main: pthread_join() failed: %s\n", strerror(err));
        return -1;
    }
    err = pthread_join(tid_eq, NULL);
    if (err) {
        printf("main: pthread_join() failed: %s\n", strerror(err));
        return -1;
    }

    err = pthread_join(tid_swap1, NULL);
    if (err) {
        printf("main: pthread_join() failed: %s\n", strerror(err));
        return -1;
    }
    err = pthread_join(tid_swap2, NULL);
    if (err) {
        printf("main: pthread_join() failed: %s\n", strerror(err));
        return -1;
    }
    err = pthread_join(tid_swap3, NULL);
    if (err) {
        printf("main: pthread_join() failed: %s\n", strerror(err));
        return -1;
    }

    list_destroy(&list);
    return 0;
}

