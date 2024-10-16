#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void list_init(Storage *list) {
  list->first = NULL;
}

void list_destroy(Storage *list) {
  Node *current = list->first;
  while (current) {
    Node *next = current->next;
    pthread_mutex_destroy(&current->sync);
    free(current);
    current = next;
  }
}

void list_insert(Storage *list, const char *str) {
  Node *new_node = malloc(sizeof(Node));
  strncpy(new_node->value, str, MAX_STR_LEN);
  new_node->next = NULL;
  pthread_mutex_init(&new_node->sync, NULL);

  pthread_mutex_lock(&new_node->sync);
  if (!list->first) {
    list->first = new_node;
  } else {
    new_node->next = list->first;
    list->first = new_node;
  }
  pthread_mutex_unlock(&new_node->sync);
}

void list_swap(Node *a, Node *b) {
  if (a == b) return;

  pthread_mutex_lock(&a->sync);
  pthread_mutex_lock(&b->sync);

  char temp[MAX_STR_LEN];
  strncpy(temp, a->value, MAX_STR_LEN);
  strncpy(a->value, b->value, MAX_STR_LEN);
  strncpy(b->value, temp, MAX_STR_LEN);

  pthread_mutex_unlock(&b->sync);
  pthread_mutex_unlock(&a->sync);
}