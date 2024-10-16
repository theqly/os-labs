#ifndef LIST_H
#define LIST_H

#include <pthread.h>

#define MAX_STR_LEN 100

typedef struct _Node {
  char value[MAX_STR_LEN];
  struct _Node *next;
  pthread_mutex_t sync;
} Node;

typedef struct _Storage {
  Node *first;
} Storage;

void list_init(Storage *list);
void list_destroy(Storage *list);
void list_insert(Storage *list, const char *str);
void list_swap(Node *a, Node *b);

#endif //LIST_H
