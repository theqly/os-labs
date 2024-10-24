#define _GNU_SOURCE
#include "myspinlock.h"


void myspinlock_init(myspinlock *s) {
  atomic_flag_clear(&s->lock);
}

void myspinlock_lock(myspinlock *s) {
  while (atomic_flag_test_and_set(&s->lock)) { }
}

void myspinlock_unlock(myspinlock *s) {
  atomic_flag_clear(&s->lock);
}
